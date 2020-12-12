#!/usr/bin/env python3
import lcm
from datetime import datetime
import robot

if __name__ == "__main__":
    print("Timestamps are in the format HH:MM:SS.microseconds")

    # Figure all message types and their channel names out
    message_types = dict([
        (n[:n.rfind("_t")].upper(), t) # see messages.lcm
        for (n, t) in robot.__dict__.items() if callable(t)
    ])

    def handler(channel, data):
        # Figure out what type we need to decode
        mt = message_types[channel]
        msg = mt.decode(data)

        # XXX: do we need to recurse when a field is a robot type? i.e.
        # brickpi_t, port_t.

        # Print when message was decoded, message type and its fields
        print(f'''
{datetime.now().strftime("%H:%M:%S.%f")}: {channel}:''')
        for field in mt.__slots__:
            print(f"\t{field}\t= {getattr(msg, field)}")

    lc = lcm.LCM()
    for channel, _ in message_types.items():
        lc.subscribe(channel, handler)

    while True:
        lc.handle()
