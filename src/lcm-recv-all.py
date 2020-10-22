#!/usr/bin/env python3
import lcm
import datetime
from dwm import position_t

def pos_handler(channel, data):
    msg = position_t.decode(data)
    print(f"(t, x, y, z, q) = ({msg.timestamp}, {msg.x}, {msg.y}, {msg.z}, {msg.q})")

def wpos_handler(channel, data):
    msg = position_t.decode(data)
    timestamp = datetime.datetime.fromtimestamp(msg.timestamp // 1e3).time()
    print(f"{timestamp}: new move order to ({msg.x}, {msg.y})");

if __name__ == "__main__":
    lc = lcm.LCM()
    lc.subscribe("POSITION", pos_handler)
    lc.subscribe("WANTED_POSITION", wpos_handler)

    try:
        while True:
            lc.handle()
    except KeyboardInterrupt:
        pass
