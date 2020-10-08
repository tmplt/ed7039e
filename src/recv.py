#!/usr/bin/env python3
# TODO: Default firewall settings are overzealous for LCM.
#       Figure out what FW setting must be appliad for LCM to work.

# Build dependencies with
#   $ lcm-gen -p dwm.lcm

import lcm
from dwm import position_t

def handler(channel, data):
    msg = position_t.decode(data)
    print(f"""
    Received message on channel {channel}.
    (x, y, z, q) = ({msg.x}, {msg.y}, {msg.z}, {msg.q})
    """)

lc = lcm.LCM()
subscription = lc.subscribe("POSITION", handler)

try:
    while True:
        lc.handle()
except KeyboardInterrupt:
    pass
