#!/usr/bin/env python3

import lcm
from dwm import position_t

def handler(channel, data):
    msg = position_t.decode(data)
    print(f"(x, y, z, q) = ({msg.timestamp}, {msg.x}, {msg.y}, {msg.z}, {msg.q})")

lc = lcm.LCM()
subscription = lc.subscribe("POSITION", handler)

try:
    while True:
        lc.handle()
except KeyboardInterrupt:
    pass
