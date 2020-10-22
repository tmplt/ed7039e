#!/usr/bin/env python3
import math
import lcm
from robot import dwm_position_t, vector2d_t

if __name__ == "__main__":
    lc = lcm.LCM()
    target_coord = None

    def wpos_handler(channel, data):
        msg = dwm_position_t.decode(data)
        target_coord = (msg.x, msg.y)

    def vec_handler(channel, data):
        if not target_coord:
            # We haven't received a any move orders yet. Do nothing.
            return

        (x, y) = target_coord
        curr = vector2d_t.decode(data)
        hypotenuse = math.sqrt((x - curr.x) ** 2 + (y - curr.y) ** 2)
        target_rad = math.asin((y - curr.y) / hypotenuse)

        error_angle = target_rad - curr.rad
        # TODO: send a message of appropriate type
        
        raise NotImplementedError

    lc.subscribe("APPROX_VECTOR", vec_handler)
    lc.subscribe("WANTED_POSITION", wpos_handler)

    try:
        while True:
            lc.handle()
    except KeyboardInterrupt:
        pass
