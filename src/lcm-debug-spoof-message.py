#!/usr/bin/env python3
import sys
import lcm
import robot
import ctypes
import re

if __name__ == "__main__":
    message_types = dict([
        (n[:n.rfind("_t")].upper(), t)
        for (n, t) in robot.__dict__.items() if callable(t)
    ])

    if len(sys.argv) < 2:
        print("give me the message to spoof")
        print(f"I can spoof messages of the types {', '.join(message_types.keys())}")
        sys.exit(1)
    channel = sys.argv[1]
    sys.argv = sys.argv[2:]

    try:
        fields = message_types[channel].__slots__
    except KeyError:
        # TODO print all possible message types again
        print("no such message")
        sys.exit(1)

    if len(sys.argv) < len(fields):
        print(f'''
missing {len(fields) - len(sys.argv)} argument(s) to fill the message
{channel}: [{", ".join(fields)}]''')
        sys.exit(1)

    msg = message_types[channel]()
    for f, arg, typestr in zip(fields, sys.argv, msg.__typenames__):
        # find the ctype of the field
        ctype = getattr(sys.modules['ctypes'],
                        'c_' + re.sub('\_t$', '', typestr))

        # before we create the ctype we must know the equivalent Python
        # class so that we convert the string argument to the correct
        # intermediate type. E.g. ‘ctypes.int_*’ -> ‘builtins.int’,
        # ‘ctypes.float’ -> ‘builtins.float’.
        # XXX: how does this work with port_t, brickpi_t, etc?
        pytype = type(ctype(0xBAADF00D).value)

        # Finally, typecast the argument, and set the field.
        setattr(msg, f, pytype(arg))

    lcm.LCM().publish(channel, msg.encode())
