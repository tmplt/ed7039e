#!/usr/bin/env python3
import sys
import lcm
import datetime, time
from robot import dwm_position_t

def parse_coord(c):
    try:
        return int(c)
    except ValueError:
        print(f"Argument '{c}' is not an integer")
        return None

def millis():
    ts = datetime.datetime.now()
    return round(time.mktime(ts.timetuple()) * 1e3
                 + ts.microsecond / 1e3)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(f"usage: {sys.argv[0]} <wanted x coord.> <wanted y coord.>")
        print("\twhere coord. is a coordinate in centimeters")
        sys.exit(1)

    p = dwm_position_t()
    (p.x, p.y) = parse_coord(sys.argv[1]), parse_coord(sys.argv[2])
    if not p.x or not p.y:
        sys.exit(1)

    p.timestamp = millis()
    lcm.LCM().publish("WANTED_POSITION", p.encode())
