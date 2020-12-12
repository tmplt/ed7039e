#!/usr/bin/env python3
import sys
import lcm
import robot
import ctypes

if __name__ == "__main__":
    message_types = dict([
        (n[:n.rfind("_t")].upper(), t) # see messages.lcm
        for (n, t) in robot.__dict__.items() if callable(t)
    ])

    if len(sys.argv) < 2:
        print(f"usage: {sys.argv[0]} <MESSAGE_TYPE> <MESSAGE_ARGUMENTS...>")
        print(f"\t where <MESSAGE_TYPE> is one of {', '.join(message_types.keys())}")
        print()
        print("For example:")
        print(f"\t{sys.argv[0]} IO_ENCODER PLATFORM PORT_A 30 1000")
        print(f"\t{sys.argv[0]} KALMAN_POSITION 3.23 4.12")
        sys.exit(1)
    channel = sys.argv[1]
    sys.argv = sys.argv[2:]

    try:
        fields = message_types[channel].__slots__
    except KeyError:
        print(f"Unknown message type '{channel}'")
        sys.exit(1)

    if len(sys.argv) != len(fields):
        print(f"Incorrect number of message arguments given. Expected {len(fields)}, but was given {len(sys.argv)}")
        print(f"for message {channel} with fields [{', '.join(fields)}]")
        sys.exit(1)

    msg = message_types[channel]()
    for field, arg, typestr in zip(fields, sys.argv, msg.__typenames__):
        try:
            # Figure out what intermediate type is needed to create the
            # underlaying ctype of the field.
            pytype = type(getattr(msg, field))
            value = pytype(arg)
        except ValueError:
            # `arg` denotes an enum. Find its symbol.
            try:
                value = getattr(getattr(sys.modules['robot'], field + '_t'), arg)
            except:
                print(f"Argument '{arg}' for field '{field}' is not an integer or a valid enum")
                sys.exit(1)

        setattr(msg, field, value)

    lcm.LCM().publish(channel, msg.encode())
