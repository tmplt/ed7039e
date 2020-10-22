#!/usr/bin/env python3

import zerocm
from dwm import position_t

def handler(channel, msg):
    print(f"(x, y, z, q) = ({msg.timestamp}, {msg.x}, {msg.y}, {msg.z}, {msg.q})")

zcm = zerocm.ZCM("udpm://239.255.76.67:7667?ttl=0")
subscription = zcm.subscribe("POSITION", position_t, handler)

try:
    while True:
        zcm.handle()
except KeyboardInterrupt:
    pass
