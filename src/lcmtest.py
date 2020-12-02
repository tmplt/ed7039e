import lcm
from robot import dwm_goal_position_t

def test():
    msg = dwm_goal_position_t()
    msg.x = 10
    msg.y = 20
    return msg.encode()

lc = lcm.LCM()
lc.publish("Example", test())

try:
    while True:
        lc.handle()
except KeyboardInterrupt:
    pass
