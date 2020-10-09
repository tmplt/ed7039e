// Build with:
//   $ lcm-gen -c dwm.lcm
//   $ gcc -d -O0 -o send send.c dwm_position_t.c -llcm

// TODO:
// * Add a timestamp to the struct (check LCM struct docs if there is a recommended data type)

#define _POSIX_C_SOURCE 200809L

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

#include <lcm/lcm.h>
#include "dwm_position_t.h"

#define TL_HEADER_LEN 0x2

enum dwm_functions {
        dwm_pos_get = 0x02, // 18B long
        dwm_cfg_get = 0x08, // 7B long
};

unsigned char buf[256];

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
        int rdlen = 0;
        while (rdlen < count) {
                int rd = read(fd, buf + rdlen, count - rdlen);
                if (rd <= 0) {
                        return -1;
                }
                rdlen += rd;
        }
        return rdlen;
}

/* Calls the remote command `fun`, checks function validity, reads function return
 * payload into the buffer starting at `buf` and return payload length.
 */
int tlv_rpc(int fd, char fun, unsigned char *buf)
{
        /* Call function. */
        char cmd[] = { fun, 0x0 };
        if (write(fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
                return -1;
        }
        tcdrain(fd);

        /* Check function return status.
         * NOTE: we should always get a 0x40 following an RPC.
         */
        int rdlen = 0;
        if ((rdlen = readn(fd, buf, TL_HEADER_LEN + 1)) < 0) {
                return -1;
        }
        if (buf[0] != 0x40 || buf[2] != 0x0) {
                puts("tlv: invalid rpc");
                return -1;
        }

        /* Read function payload. */
        if ((rdlen = readn(fd, buf, TL_HEADER_LEN)) < 0) {
                return -1;
        }
        int val_len = buf[1];
        if ((rdlen = readn(fd, buf + rdlen, val_len)) < 0) {
                return -1;
        }
        
        return rdlen;
}

int main(int argc, char **argv)
{
        /* Initialize LCM. */
        lcm_t *lcm = lcm_create(NULL);
        if (!lcm) {
                return 1;
        }

        /* Open a serial connection with the DWM. */
        char *portname = "/dev/serial/by-id/usb-SEGGER_J-Link_000760109125-if00";
        int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
        if (fd < 0 || configure_tty(fd) < 0) {
                goto error;
        }

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
                int tlv_len = 0;
                if ((tlv_len = tlv_rpc(fd, dwm_pos_get, buf)) < 0) {
                        puts("tlv error");
                        continue;
                }
                if (buf[0] != 0x41) {
                        printf("recvd unexpected payload type %02x\n", buf[0]);
                        continue;
                }

                /* Check the time. */
                clock_gettime(CLOCK_REALTIME, &ts);
                pos.timestamp = (ts.tv_sec * 1e3) + round(ts.tv_nsec / 1e3f);
                
                /* Copy payload into struct.
                 * XXX: `dwm_position_t` is padded with 3B,
                 * but the below works in this case.
                 */
                assert(tlv_len == 13);
                memcpy(&pos.x, buf + TL_HEADER_LEN, tlv_len);

                /* Publish payload on appropriate channel. */
                dwm_position_t_publish(lcm, "POSITION", &pos);

                nanosleep(&sleep_duration, NULL);  
        }

        goto cleanup;

error:
        printf("%s\n", strerror(errno));

cleanup:
        if (fd != -1) {
                close(fd);
        }
        lcm_destroy(lcm);
        return 0;
}
