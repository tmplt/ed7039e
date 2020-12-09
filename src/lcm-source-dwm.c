#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <sys/poll.h>
#include <signal.h>

#include <lcm/lcm.h>
#include "robot_io_position_t.h"
#include "robot_io_acceleration_t.h"

#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define ERROR(...) {                                            \
                fprintf(stderr, "%s:%d: ", __func__, __LINE__); \
                fprintf(stderr, __VA_ARGS__);                   \
                fprintf(stderr, "\n");                          \
        }
#define BKPT raise(SIGTRAP);
#define BUFFER_SIZE 256
#define READ_TIMEOUT 500        /* milliseconds */
#define GRAVITY_CONSTANT 9.81   /* m/s^2 */

enum serial_modes {
        serial_mode_tlv,        /* Not implemented */
        serial_mode_shell,
};

typedef struct {
        lcm_t *lcm;
        char buf[BUFFER_SIZE];
        int fd;
} ctx_t;                        /* DWM context */

int read_until(int fd, char *str, char *buf);
int set_serial_mode(int fd, enum serial_modes mode);
int readt(int fd, void* buf, size_t count);
int tlv_rpc(int fd, char fun, char *buf, char *respbuf);

/* poll(2) wrapper around read(2). */
int readt(int fd, void* buf, size_t count)
{
        struct pollfd fds = {
                .fd = fd,
                .events = POLLIN,
        };

        int retval;
        if ((retval = poll(&fds, 1, READ_TIMEOUT)) == 0) {
                errno = ETIMEDOUT;
                return -ETIMEDOUT;
        } else if (retval < 0) {
                ERROR("poll failure: %d", retval);
                return retval;
        } else if (retval > 0 && fds.revents & POLLIN) {
                return read(fd, buf, count);
        }

        ERROR("poll failure: %d", retval);
        return retval;
}

/* Transitions the DWM accessible via file descriptor `fd` to the specifid mode. */
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
        if ((retval = read_until(fd, "dwm> ", buf)) < 0) {
                ERROR("read_until failure: %d, %s", retval, strerror(errno));
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

        /* Configure serial attributes, baud rate. */
        cfmakeraw(&tty);
        cfsetospeed(&tty, B115200);
        cfsetispeed(&tty, B115200);

        if (tcsetattr(fd, TCSANOW, &tty) != 0) {
                return -1;
        }

        return 0;
}

/* Reads bytes from the file descriptor `fd` into the buffer starting at `buf` until
 * the string `str` is found in `buf`. May read past `str` from `fd` if bytes are
 * available during read(2). Returns the number of byter read, or an read(2) error
 * code.
 */
int read_until(int fd, char *str, char *buf)
{
        /* Because of the strstr below, and the possibility of readt returning
         * fewer bytes than the total sum read on the last read_until call,
         * the buffer is conveniently zeroed so the function doesn't
         * prematurely return, having strstr-matched with outdated data.
         */
        memset(buf, 0, BUFFER_SIZE);

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

/* Computes MAX(`a` - `b`, 0) represented as timespec structs. */
void timespec_diff(struct timespec *a, struct timespec *b, struct timespec *r)
{
        r->tv_sec = MAX(a->tv_sec - b->tv_sec, 0);
        r->tv_nsec = MAX(a->tv_nsec - b->tv_nsec, 0);
}

int publish_pos(ctx_t *ctx, int64_t timestamp)
{
        int retval;
        int x = 0, y = 0, z = 0, q = 0;
        if ((retval = sscanf(ctx->buf, "%*s\napg: x:%d y:%d z:%d qf:%d\r\n",
                             &x, &y, &z, &q)) != 4) {
                ERROR("sscanf failure: %d", retval);
                return -ENOMSG;
        }

        robot_io_position_t pos = {
                .timestamp = timestamp,
                .x = x / 1e3,
                .y = y / 1e3,
                .z = z / 1e3,
                .q = q,

        };
        return robot_io_position_t_publish(ctx->lcm, "IO_POSITION", &pos);
}

/* Converts the raw LIS2DH12 acceleration register value `v` to SI unit (m/s^2). */
float raw_acc_reg_to_si(int64_t v)
{
        /* XXX: reportedly correct according to
         * <https://decaforum.decawave.com/t/dwm1001-accelerometer-readings/1202/6>.
         * Assumes that +-2g scale is configured for.
         * XXX: documentation says that `pow(2, 6)` should only be used when in low-power
         * +-8g scale, but the `query("av")` data isn't in range.
         */
        return (v / pow(2, 6)) * 0.004 * GRAVITY_CONSTANT;
}

int publish_acc(ctx_t *ctx, int64_t timestamp)
{
        int retval;
        int x = 0, y = 0, z = 0;
        if ((retval = sscanf(ctx->buf, "%*s\nacc: x = %d, y = %d, z = %d\r\n",
                             &x, &y, &z)) != 3) {
                ERROR("sscanf failure: %d", retval);
                return -ENOMSG;
        }

        robot_io_acceleration_t acc = {
                .timestamp = timestamp,
                .x = raw_acc_reg_to_si(x),
                .y = raw_acc_reg_to_si(y),
                .z = raw_acc_reg_to_si(z),
        };
        return robot_io_acceleration_t_publish(ctx->lcm, "IO_ACCELERATION", &acc);
}

/* Queries the DWM context `ctx` with the function `fun` and forwards the response
 * to the parser and publisher `pap`.
 */
int query(ctx_t *ctx, char *fun, int (*pap)(ctx_t*, int64_t))
{
        /* Execute the function on the DWM. */
        if (write(ctx->fd, fun, strlen(fun)) < strlen(fun)) {
                ERROR("failed to query \"%s\"", fun);
                return -EBADF;
        }

        /* Read the response of the DWM. */
        if (read_until(ctx->fd, "\r\ndwm> ", ctx->buf) < 0) {
                ERROR("could not read query response");
                return -EBADF;
        }

        /* Generate a timestamp. */
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);

        /* Forward response and timestamp (in milliseconds) to the parser and publisher. */
        return pap(ctx, (ts.tv_sec * 1e3) + round(ts.tv_nsec / 1e6f));
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
                ERROR("failed to initialized LCM");
                return 1;
        }
        /* Open a serial connection to the DWM. */
        ctx.fd = open(argv[1], O_RDWR | O_NOCTTY | O_SYNC | O_NONBLOCK);
        if (ctx.fd < 0 || configure_tty(ctx.fd) < 0) {
                ERROR("failed to configure serial: %s\n", strerror(errno));
                goto cleanup;
        }

        /* Change serial mode. */
        while (set_serial_mode(ctx.fd, serial_mode_shell) < 0) {
                ERROR("failed to enter shell serial mode; retrying...");
        }

        struct timespec timer = {
                .tv_sec = 0,

                /* NOTE(70): effectively gives us a 10Hz polling rate,
                 * because query("av") takes ~33ms to run.
                 * XXX: depended on DWM speed, host system, or both (likely).
                 * DWM bottleneck?
                 */
                .tv_nsec = 70 * 1e6,
        };
        struct timespec now = {0, 0}, last = {0, 0}, res = {0, 0};

        for (;;) {
                /* Is it time to query the position? */
                clock_gettime(CLOCK_REALTIME, &now);
                timespec_diff(&now, &last, &res);
                if (res.tv_sec > timer.tv_sec || res.tv_nsec >= timer.tv_nsec) {
                        query(&ctx, "apg\r", publish_pos);
                        last = now;
                }

                query(&ctx, "av\r", publish_acc);
        }

cleanup:
        if (ctx.fd != -1) {
                close(ctx.fd);
        }
        if (ctx.lcm != NULL) {
                lcm_destroy(ctx.lcm);
        }
        return 0;
}
