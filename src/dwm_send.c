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

#include <lcm/lcm.h>
#include "dwm_position_t.h"
#include "dwm_acceleration_t.h"

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
        pthread_mutex_t lock;
        lcm_t *lcm;
        unsigned char buf[BUFFER_SIZE];
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

        /* char cmd[] = { 0x0D, 0x0D }; */
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

        if (read_until(fd, "dwm> ", NULL) < 0) {
                puts("set_serial_mode: read_until failure");
                return -1;
        }

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
        char tmpbuf[BUFFER_SIZE];
        char *b = buf != NULL ? buf : tmpbuf;
        if (b == tmpbuf) {
                memset(tmpbuf, 0, sizeof(tmpbuf));
        }
        int rdlen = 0;
        while (!strstr(b, str)) {
                int rd = 0;
                if ((rd = readt(fd, b + rdlen, BUFFER_SIZE)) < 0) {
                        return rd;
                }
                rdlen += rd;
        }

        return 0;
}

/* Calls the remote command `fun`, checks function validity, reads function return
 * payload into the buffer starting at `buf` and return payload length.
 */
int tlv_rpc(int fd, char fun, char *buf, char *respbuf)
{
        /* Call function. */
        char format[] = "tlv %02x 00\r\n";
        char cmd[sizeof("tlv 02 00\r\n")];
        snprintf(cmd, sizeof(cmd), format, fun);

        /* The interactive shell echoes back written bytes,
         * which it expects us to read before processing next incoming bytes.
         */
        int retval = 0;
        for (int i = 0; i < strlen(cmd); i++) {
                char b;         /* XXX: required instead of buf: lest "OUTPUT FRAME" is shredded on consequent calls. Why? */
                if (write(fd, cmd + i, 1) < 0 ||
                    (retval = readt(fd, &b, 1)) < 0) {
                        printf("tlv_rpc: could not call function: %s\n", strerror(errno));
                        return -1;
                }
        }

        /* Read out full response. */
        if ((retval = read_until(fd, "\r\ndwm> ", buf)) < 0) {
                printf("tlv_rpc: could not read out full response: %s\n", strerror(errno));
                return -1;
        }

        if (!strstr(buf, "OUTPUT FRAME")) {
                puts("tlv_rpc: missing expected response frame header");
                return -1;
        }

        /* Assume we made a correct function call and find the prefix location. */
        char ok_funcall_prefix[] = "40 01 00";
        if ((buf = strstr(buf, ok_funcall_prefix)) == NULL) {
                puts("tlv_rpc: did not find expected OK prefix in repsonse frame");
                return -1;
        }
        buf += sizeof(ok_funcall_prefix);
        
        /* Convert the hexadecimal payload to binary in-place. */
        int payload_hex_len = strstr(buf, "\r\ndwm> ") - buf;
        if (payload_hex_len <= 0) {
                puts("tlv_rpc: could not find response suffix");
                return -1;
        }
        int i = 0;
        for (; i * 3 < payload_hex_len; i++) {
                /* Read the hexadecimal byte from the buffer and write it back in binary. */
                sscanf(buf + (i * 3), "%02hhx", respbuf + i);
        }

        return i - TL_HEADER_LEN; /* return payload length */
}

void timespec_diff(struct timespec *a, struct timespec *b, struct timespec *r)
{
        r->tv_sec = a->tv_sec - b->tv_sec;
        r->tv_nsec = a->tv_nsec - b->tv_nsec;
}

void* poll_position_loop(void *arg)
{
        ctx_t *ctx = (ctx_t*)arg;
        
        /* Configure a periodic sleep for 100ms. We poll at 10Hz */
        struct timespec sleep_duration = {
                .tv_sec = 0,
                .tv_nsec = 100 * 1e6,
        };
        struct timespec ts, start, end, res;
        dwm_position_t pos;
        memset(&pos, 0, sizeof(pos));
        char respbuf[BUFFER_SIZE];      /* XXX: required? */
                
        for (;;) {
                /* Query measured position. */
                pthread_mutex_lock(&ctx->lock);
                clock_gettime(CLOCK_REALTIME, &start);
                int tlv_len = 0;
                int error = 0;
                if ((tlv_len = tlv_rpc(ctx->fd, dwm_pos_get, ctx->buf, respbuf)) < 0) {
                        error = 1;
                }
                if (respbuf[0] != 0x41 && !error) {
                        printf("read unexpected payload type %02x\n", respbuf[0]);
                        error = 1;
                }
                pthread_mutex_unlock(&ctx->lock);
                if (error) {
                        continue;
                }

                /* Check the time. */
                clock_gettime(CLOCK_REALTIME, &ts);
                pos.timestamp = (ts.tv_sec * 1e3) + round(ts.tv_nsec / 1e3f);
                
                /* Copy payload into struct.
                 * XXX: `dwm_position_t` is padded with 3B,
                 * but the below works in this case.
                 * TODO: dont memcpy, assign each struct member instead (or memcpy to them)
                 */
                assert(tlv_len == 13);
                memcpy(&pos.x, respbuf + TL_HEADER_LEN, tlv_len);

                /* Publish payload on appropriate channel. */
                dwm_position_t_publish(ctx->lcm, "POSITION", &pos);

                /* Calculate how long we should sleep. */
                clock_gettime(CLOCK_REALTIME, &end);
                timespec_diff(&end, &start, &res);
                timespec_diff(&sleep_duration, &res, &res);
                printf("sleeping for %lld.%.9ld\n", (long long)res.tv_sec, res.tv_nsec);

                nanosleep(&res, NULL);
        }

        return NULL;
}

void* poll_acceleration_loop(void *arg)
{
        ctx_t *ctx = (ctx_t*)arg;
        
        /* Configure a periodic sleep for 100ms. We poll at 10Hz */
        /* struct timespec sleep_duration = { */
        /*         .tv_sec = 0, */
        /*         .tv_nsec = 100 * 1e6, */
        /* }; */
        /* /\* struct timespec ts; *\/ */
        /* dwm_acceleration_t acc; */
        /* memset(&acc, 0, sizeof(acc)); */
        for (;;) {
                /* Query measured acceleration. */
                /* pthread_mutex_lock(&ctx.lock); */
                /* clock_t t = clock(); */
                /* t = clock() - t; */
                /* printf("%f\n", ((double) t)/CLOCKS_PER_SEC); */
                /* pthread_mutex_unlock(&ctx.lock); */
                break;
                /* nanosleep(&sleep_duration, NULL); */

        }
        
        return NULL;
}

void ctx_destroy(ctx_t *ctx)
{
        if (ctx->fd != -1) {
                close(ctx->fd);
        }
        lcm_destroy(ctx->lcm);
        pthread_mutex_destroy(&ctx->lock);
}

int main(int argc, char **argv)
{
        if (argc != 2) {
                printf("usage: %s <serial-file>\n", argv[0]);
                return 1;
        }

        ctx_t ctx;

        if (pthread_mutex_init(&ctx.lock, NULL) != 0) {
                printf("failed to init context mutex: %s", strerror(errno));
                return 1;
        }

        /* Initialize LCM. */
        ctx.lcm = lcm_create(NULL);
        if (!ctx.lcm) {
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

        /* Spawn polling threads: one reads position, the other acceleration. */
        pthread_t post, acct;
        if (pthread_create(&post, NULL, poll_position_loop, &ctx) != 0 ||
            pthread_create(&acct, NULL, poll_acceleration_loop, &ctx) != 0) {
                printf("failed to start thread: %s\n", strerror(errno));
        }

        pthread_join(post, NULL);
        pthread_join(acct, NULL);

        // TODO: call pthread_cancel if any thread fails

cleanup:
        ctx_destroy(&ctx);
        return 0;
}
