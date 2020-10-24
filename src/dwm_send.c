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

#include <lcm/lcm.h>
#include "dwm_position_t.h"
#include "dwm_acceleration_t.h"

#define TL_HEADER_LEN 0x2

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
        unsigned char buf[256];
        int fd;
} ctx_t;

int read_until(int fd, char *str, char *buf);
int set_serial_mode(int fd, enum serial_modes mode);

int set_serial_mode(int fd, enum serial_modes mode)
{
        assert(mode == serial_mode_shell);
        
        char b = 0x0;
        char check[] = { 0x0D };
        if (write(fd, check, sizeof(check)) != sizeof(check) ||
            read(fd, &b, 1) < 0) {
                printf("failed to probe serial mode");
                return -1;
        }
        
        if (b != 0x0D) {
                puts("DWM not in shell mode. Requesting change...");

                char cmd[] = { 0x0D, 0x0D };
                if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
                        printf("failed to request serial mode");
                        return -1;
                }
                tcdrain(fd);
        }

        /* Ensure an expected serial state after this function.  */
        while (read_until(fd, "dwm> ", NULL) < 0);
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

/* Reads `count` bytes from the file descriptor `fd` into the buffer starting at `buf`.
 * Returns -1 on error.
 */
int readn(int fd, unsigned char *buf, size_t count)
{
        size_t rdlen = 0;
        while (rdlen < count) {
                int rd = read(fd, buf + rdlen, count - rdlen);
                if (rd <= 0) {
                        return -1;
                }
                rdlen += rd;
        }
        return rdlen;
}

/* Reads bytes from the file descriptor `fd` into the buffer starting at `buf` until
 * the string `str` is read. `buf` can be NULL. Returns -1 on error.
 */
int read_until(int fd, char *str, char *buf)
{
        fd_set read_fds, write_fds, except_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_ZERO(&except_fds);
        FD_SET(fd, &read_fds);

        struct timeval timeout = {
                .tv_sec = 1,
                .tv_usec = 0,
        };
        
        size_t idx = 0, cidx = 0;
        char ch;
        while (cidx < strlen(str)) {
                /* Read one byte at a time into a conditional buffer. */
                if (select(fd + 1, &read_fds, &write_fds, &except_fds, &timeout) == 1) {
                        if (read(fd, &ch, 1) < 0) {
                                return -1;
                        }                        
                } else {
                        puts("read_until: timeout");
                        return -1;
                }

                if (buf != NULL) {
                        buf[idx++] = ch;
                }

                /* Count how much of `str` we have read. */
                if (str[cidx] == ch) {
                        cidx++;
                } else {
                        cidx = 0;
                }
        }

        return 0;
}

/* Calls the remote command `fun`, checks function validity, reads function return
 * payload into the buffer starting at `buf` and return payload length.
 */
int tlv_rpc(int fd, char fun, unsigned char *buf)
{
        /* Call function. */
        char format[] = "tlv %02x 00%c";
        char cmd[sizeof(format) - 1];
        snprintf(cmd, sizeof(cmd), format, fun, 0x0D);

        /* The interactive shell echoes back written bytes,
         * which it expects us to read before processing next incoming bytes.
         */
        for (int i = 0; i < strlen(cmd); i++) {
                if (write(fd, cmd + i, 1) < 0 ||
                    read(fd, buf, 1) < 0) {
                        return -1;
                }
        }

        /* Discard useless prepending bytes. */
        if (read_until(fd, "\x0D", NULL) < 0 || read(fd, buf, 1) < 0) {
                return -1;
        }

        /* Read out function response. */
        if (read_until(fd, "\x0D", buf) < 0) {
                return -1;
        }

        /* Assume we made a correct function call for implementation brevity. */
        char ok_funcall_prefix[] = "40 01 00";
        if (strncmp(buf, ok_funcall_prefix, strlen(ok_funcall_prefix)) != 0) {
                puts("tlv_rpc: did not find expected OK prefix in repsonse frame");
                return -1;
        }

        /* Convert the hexadecimal payload to binary in-place. */
        int payload_hex_len = strlen(buf + sizeof(ok_funcall_prefix) + 1);
        int i = 0;
        for (; i * 3 < payload_hex_len; i++) {
                /* Read the hexadecimal byte from the buffer and write it back in binary.
                 * NOTE: We'll have a safety buffer `sizeof(ok_funcall_prefix)` between
                 * the data being processed.
                 */
                sscanf(buf + sizeof(ok_funcall_prefix) + (i * 3), "%02hhx", buf + i);
        }

        /* Reset serial to a known state (no pending bytes to read). */
        if (read_until(fd, "dwm> ", NULL) < 0) {
                printf("failed to reset serial state");
                return -1;
        }

        return i - TL_HEADER_LEN; /* return payload length */
}

void* poll_position_loop(void *arg)
{
        ctx_t *ctx = (ctx_t*)arg;
        
        /* Configure a periodic sleep for 100ms. We poll at 10Hz */
        struct timespec sleep_duration = {
                .tv_sec = 0,
                .tv_nsec = 100 * 1e6,
        };
        struct timespec ts;
        dwm_position_t pos;
        memset(&pos, 0, sizeof(pos));
        for (;;) {
                /* Query measured position. */
                pthread_mutex_lock(&ctx->lock);
                int tlv_len = 0;
                int error = 0;
                if ((tlv_len = tlv_rpc(ctx->fd, dwm_pos_get, ctx->buf)) < 0) {
                        puts("tlv_rpc error. Trying again...");
                        error = 1;
                }
                if (ctx->buf[0] != 0x41) {
                        printf("recvd unexpected payload type %02x\n", ctx->buf[0]);
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
                memcpy(&pos.x, ctx->buf + TL_HEADER_LEN, tlv_len);

                /* Publish payload on appropriate channel. */
                dwm_position_t_publish(ctx->lcm, "POSITION", &pos);

                nanosleep(&sleep_duration, NULL);
        }

        return NULL;
}

void* poll_acceleration_loop(void *arg)
{
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
        if (set_serial_mode(ctx.fd, serial_mode_shell) < 0) {
                puts("failed to enter shell serial mode");
                goto cleanup;
        }

        /* Spawn polling threads: one reads position, the other acceleration. */
        pthread_t post; //, acct;
        if (pthread_create(&post, NULL, poll_position_loop, &ctx) != 0) { // ||
            /* pthread_create(&acct, NULL, poll_acceleration_loop, NULL) != 0) { */
                printf("failed to start thread: %s\n", strerror(errno));
        }

        pthread_join(post, NULL);
        /* pthread_join(acct, NULL); */
        /* poll_position_loop(NULL); */
        /* poll_acceleration_loop(NULL); */
        /* poll_position_loop(NULL); */
        /* poll_position_loop(NULL); */

        // TODO: call pthread_cancel if any thread fails

cleanup:
        ctx_destroy(&ctx);
        return 0;
}
