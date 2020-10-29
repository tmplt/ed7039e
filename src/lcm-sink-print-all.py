#!/usr/bin/env python3
import lcm
import datetime
from robot import dwm_position_t, dwm_acceleration_t

def pos_handler(channel, data):
    msg = dwm_position_t.decode(data)
    print(f"POS (t, x, y, z, q) = ({msg.timestamp}, {msg.x}, {msg.y}, {msg.z}, {msg.q})")

def wpos_handler(channel, data):
    msg = dwm_position_t.decode(data)
    timestamp = datetime.datetime.fromtimestamp(msg.timestamp // 1e3).time()
    print(f"{timestamp}: new move order to ({msg.x}, {msg.y})");

def acc_handler(channel, data):
    msg = dwm_acceleration_t.decode(data)
    print(f"ACC (t, x, y, z) = ({msg.timestamp}, {msg.x}, {msg.y}, {msg.z})")

if __name__ == "__main__":
    lc = lcm.LCM()
    lc.subscribe("POSITION", pos_handler)
    lc.subscribe("ACCELERATION", acc_handler)
    lc.subscribe("WANTED_POSITION", wpos_handler)

    try:
        while True:
            lc.handle()
    except KeyboardInterrupt:
        pass
