// Build with:
//   $ lcm-gen -c dwm.lcm
//   $ gcc -o send send.c dwm_position_t.c -llcm

#include <lcm/lcm.h>
#include "dwm_position_t.h"

int main(int argc, char **argv)
{
        lcm_t *lcm = lcm_create(NULL);
        if (!lcm) return 1;

        // Publish some bogus data
        dwm_position_t data = {
                .x = 1,
                .y = 2,
                .z = 3,
                .q = 100,
        };
        dwm_position_t_publish(lcm, "POSITION", &data);

        lcm_destroy(lcm);
        return 0;
}
