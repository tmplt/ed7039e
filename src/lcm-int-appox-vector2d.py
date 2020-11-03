#!/usr/bin/env python3
import lcm
from robot import dwm_position_t, vector2d_t

if __name__ == "__main__":
    lc = lcm.LCM()

    def handler(channel, data):
        msg = dwm_position_t.decode(data)
        # TODO: approximate a 2D vector from noisy position data

        vector = vector2d_t()
        # TODO: fill `vector` with calculated data

        raise NotImplementedError
        
        lc.publish("APPROX_VECTOR", vector.encode)
    
    subs = lc.subscribe("POSITION", handler)

    try:
        while True:
            lc.handle()
    except KeyboardInterrupt:
        pass
