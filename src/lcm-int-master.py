#!/usr/bin/env python3
import lcm
import robot.master_t as master_cmd
import robot.robot_mode_t as modes
import robot.system_state_t as ss_t

current_mode = None
system_state = None
lc = None

def handler(channel, data):
    pass

def enter(mode) -> bool:
    global current_mode, lc
    # Sanity check: the only valid chain of modes is
    #
    #    (G2G -> line -> obj) -> (G2G -> line -> obj)
    #
    # From the above:
    # - G2G ALWAYS follows a obj
    # - line ALWAYS follows a G2G
    # - obj ALWAYS follows a line
    valid_transitions = [
        # (to, from)
        (modes.DWM_MODE, None),
        (modes.DWM_MODE, modes.OBJECT_MODE),
        (modes.LINEFOLLOW_MODE, modes.DWM_MODE),
        (modes.OBJECT_MODE, modes.LINEFOLLOW_MODE)
    ]
    if (mode, current_mode) not in valid_transitions:
        print(f"Invalid transition: from {current_mode} to {mode}! Ignoring.")
        return False

    cmd = master_cmd()
    cmd.robot_mode = mode
    lc.publish("MASTER", cmd.encode())
    current_mode = mode
    return True

def close_enough():
    # XXX: stub
    # here we need to know the system state and the location of the stations.
    if system_state == None:
        return False

    return False

def line_found(_channel, _data):
    # When following the line we may get lot of these messages,
    # but we need not re-enter the mode.
    if current_mode == modes.LINEFOLLOW_MODE:
        return

    if close_enough():
        enter(modes.LINEFOLLOW_MODE)

def arrowhead_handler(_channel, _data):
    if current_mode != None:
        print("Ignoring arrowhead message; we are already processing a request.")
        return

    # Start the chain
    enter(modes.DWM_MODE)

def system_state_handler(_channel, data):
    system_state = ss_t.decode(data)

if __name__ == "__main__":
    lc = lcm.LCM()

    lc.subscribe("IO_ARROWHEAD", arrowhead_handler)
    lc.subscribe("SYSTEM_STATE", system_state_handler)
    lc.subscribe("IO_LINE_FOLLOWER", line_found)
    lc.subscribe("ADHOC_ARM", lambda _c, _d: enter(modes.DWM_MODE))

    while True:
        lc.handle()
