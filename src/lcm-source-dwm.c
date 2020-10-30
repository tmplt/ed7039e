#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <sys/select.h>
#include <errno.h>
#include <signal.h>

#include <lcm/lcm.h>
#include "robot_dwm_position_t.h"
#include "robot_dwm_acceleration_t.h"

#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define BKPT raise(SIGTRAP);
#define TL_HEADER_LEN 0x2
#define BUFFER_SIZE 256

enum dwm_functions {
        dwm_pos_get = 0x02, // 18B long
        dwm_cfg_get = 0x08, // 7B long
};

enum serial_modes {
        serial_mode_tlv,
        serial_mode_shell,
};

typedef struct {
        lcm_t *lcm;
        char buf[BUFFER_SIZE];
        int fd;
} ctx_t;

int read_until(int fd, char *str, char *buf);
int set_serial_mode(int fd, enum serial_modes mode);
int readt(int fd, void* buf, size_t count);
int tlv_rpc(int fd, char fun, char *buf, char *respbuf);

/* read(2) wrapper with a pre-configured timeout. */
int readt(int fd, void* buf, size_t count)
{
        fd_set read_fds, write_fds, except_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_ZERO(&except_fds);
        FD_SET(fd, &read_fds);

        struct timeval timeout = {
                .tv_sec = 0,
                .tv_usec = 500 * 1e3, /* 500ms */
        };

        if (select(fd + 1, &read_fds, &write_fds, &except_fds, &timeout) != 1) {
                errno = ETIMEDOUT;
                return -ETIMEDOUT;
        }

        return read(fd, buf, count);
}

int set_serial_mode(int fd, enum serial_modes mode)
{
        assert(mode == serial_mode_shell);

        char respb = 0;
        int retval;

retry:
        /* We need to write "\r\r" within the span of a
         * second to enter shell mode. But if we write the
         * bytes too fast, they will be interpreted as a TLV
         * call instead. So we first write "\r", wait for the
         * timeout (as configured in readt), and then write the
         * second "\r".
         */
        if (write(fd, "\r", 1) < 1) {
                return -1;
        }
        if ((retval = readt(fd, &respb, 1)) == -ETIMEDOUT) {
                if (write(fd, "\r", 1) < 1 ||
                    readt(fd, &respb, 1) < 1) {
                        return -1;
                }
        }
        
        /* After writing "\r\r" serial may respond with:
         *   0x0  => the device has been woken up from sleep; and
         *   0x40 => the device has no idea what to do.
         * On either of these bytes, just try again.
         * When bytes are echoed back we have entered shell mode.
         */
        if (retval && (respb == 0x0 || respb == 0x40)) {
                goto retry;
        } else if (retval < 0) {
                return -1;
        }

        char buf[BUFFER_SIZE];
        if (read_until(fd, "dwm> ", buf) < 0) {
                puts("set_serial_mode: read_until failure");
                return -1;
        }

        /* Ensure we are in a known state by discarding all
         * incoming data.
         */
        while (readt(fd, buf, sizeof(buf)) != -ETIMEDOUT);

        return 0;
}

static int configure_tty(int fd)
{
        struct termios tty;

        if (tcgetattr(fd, &tty) < 0) {
                return -1;
        }

        // configure serial attributes, baud rate
        cfmakeraw(&tty);
        cfsetospeed(&tty, B115200);
        cfsetispeed(&tty, B115200);

        if (tcsetattr(fd, TCSANOW, &tty) != 0) {
                return -1;
        }

        return 0;
}

/* Reads bytes from the file descriptor `fd` into the buffer starting at `buf` until
 * the string `str` is read. `buf` can be NULL. Returns -1 on error.
 */
int read_until(int fd, char *str, char *buf)
{
        int rdlen = 0;
        do {
                int rd = 0;
                if ((rd = readt(fd, buf + rdlen, BUFFER_SIZE)) < 0) {
                        return rd;
                }
                rdlen += rd;
        } while (!strstr(buf, str));

        return rdlen;
}

void timespec_diff(struct timespec *a, struct timespec *b, struct timespec *r)
{
        r->tv_sec = MAX(a->tv_sec - b->tv_sec, 0);
        r->tv_nsec = MAX(a->tv_nsec - b->tv_nsec, 0);
}

void poll_position_loop(ctx_t *ctx)
{
        int retval;
        robot_dwm_position_t pos;
        memset(&pos, 0, sizeof(pos));

        char cmd[] = "apg\r";

        if (write(ctx->fd, cmd, sizeof(cmd)) < sizeof(cmd)) {
                puts("failed to write");
                return;
        }

        /* Read out full response. */
        if ((retval = read_until(ctx->fd, "\r\ndwm> ", ctx->buf)) < 0) {
                printf("could not read out full response: %s\n", strerror(errno));
                return;
        }

        char *buf;
        if ((buf = strstr(ctx->buf, "apg:")) == NULL) {
                puts("could not find substring \"apg:\"");
                return;
        }

        if ((retval = sscanf(buf, "apg: x:%ld y:%ld z:%ld qf:%d\r\n", &pos.x, &pos.y, &pos.z, &pos.q)) < 4) {
                printf("pos: failed to sscanf buffer: %d fields correctly read out\n", retval);
                return;
        }

        /* Check the time. */
        {
                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);
                pos.timestamp = (ts.tv_sec * 1e3) + round(ts.tv_nsec / 1e3f);
        }

        /* Publish payload on appropriate channel. */
        robot_dwm_position_t_publish(ctx->lcm, "POSITION", &pos);
}

void poll_acceleration_loop(ctx_t *ctx)
{
        int retval;
        robot_dwm_acceleration_t acc;
        
        /* Query measured acceleration. */
        memset(ctx->buf, 0, sizeof(ctx->buf));

        char cmd[] = "av\r";

        if (write(ctx->fd, cmd, sizeof(cmd)) < sizeof(cmd)) {
                puts("failed to write");
                return;
        }

        /* Read out full response. */
        if ((retval = read_until(ctx->fd, "\r\ndwm> ", ctx->buf)) < 0) {
                printf("could not read out full response: %s\n", strerror(errno));
                return;
        }

        char *buf;
        if ((buf = strstr(ctx->buf, "acc:")) == NULL) {
                puts("could not find substring \"acc:\"");
                return;
        }
        if ((retval = sscanf(buf, "acc: x = %ld, y = %ld, z = %ld\r\n", &acc.x, &acc.y, &acc.z)) < 3) {
                printf("failed to sscanf buffer: %d fields correctly read out\n", retval);
                return;
        }

        /* Check the time. */
        {
                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);
                acc.timestamp = (ts.tv_sec * 1e3) + round(ts.tv_nsec / 1e3f);
        }

        robot_dwm_acceleration_t_publish(ctx->lcm, "ACCELERATION", &acc);
}

void ctx_destroy(ctx_t *ctx)
{
        if (ctx->fd != -1) {
                close(ctx->fd);
        }
        lcm_destroy(ctx->lcm);
}

int main(int argc, char **argv)
{
        if (argc < 2) {
                printf("usage: %s <serial-device> [lcm-provider]\n", argv[0]);
                return 1;
        }

        /* Initialize context */
        ctx_t ctx;
        memset(ctx.buf, 0, sizeof(ctx.buf));
        ctx.lcm = lcm_create(argc >= 3 ? argv[2] : NULL);
        if (!ctx.lcm) {
                puts("failed to initialize LCM");
                return 1;
        }
        /* Open a serial connection to the DWM. */
        ctx.fd = open(argv[1], O_RDWR | O_NOCTTY | O_SYNC);
        if (ctx.fd < 0 || configure_tty(ctx.fd) < 0) {
                printf("failed to configure serial: %s\n", strerror(errno));
                goto cleanup;
        }

        /* Change serial mode. */
        while (set_serial_mode(ctx.fd, serial_mode_shell) < 0) {
                puts("failed to enter shell serial mode; retrying...");
        }

        for (;;) {
                poll_position_loop(&ctx);
                poll_acceleration_loop(&ctx);
        }

cleanup:
        ctx_destroy(&ctx);
        return 0;
}
