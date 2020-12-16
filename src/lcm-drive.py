from robot import dmw_position_t, dmw_goal_position_t
from __future__ import print_function   # use python 3 syntax but make it compatible with python 2
from __future__ import division         # 
from numpy import clip

import motor-conf
import lcm
import math
import brickpi3                         # import the BrickPi3 drivers
import time


BP = motor-conf.BOT_BP      # Create an instance of the BrickPi3 class. BP will be the BrickPi3 object.
gp = [0, 0]             # gp stands for the goal position and should be fed from decawave module or a lcm node,
                        # gp = [desired x, desired y]
cp = [0, 0, 0]          # cp stands for the current position and should be fed from decawave module or a lcm node, 
                        # cp = [current x, current y, current Theta]

def ThetaError(Goal, Current):
    Tdiff = Goal - Current                                      #calculate the difference
    ThetaError = math.atan2(math.sin(Tdiff), math.cos(Tdiff))   #maps the difference in to (-\pi, \pi]
    return ThetaError

def WheelVelocities(V, w):
    L = 21                      #the width of the robot in [cm] 
    vr = V + L*w/2              #calculate right wheel's velocity
    vl = V - L*w/2              #calculate left wheel's velocity
    WV = [vr, vl]               #WV stands for wheelVelocities
    WV = clip(WV, -100, 100)    #saturate the velocity value from -100 to 100, This is the input span for the servoes
    return WV

def cart2polar(CurrentX, CurrentY, XGoal, YGoal):
    '''
    With this function we calculate the distance and the angel robot needs to move
    i.e. we map the goal point in to polar coordinates relative body frame
    '''
    x = XGoal - CurrentX        #calculate the (x, y) differences
    y = YGoal - CurrentY
    distance = math.sqrt(x**2 + y**2)
    angle = math.atan2(y, x)
    return [distance, angle]

def drive(cp, gp):
    v = 100
    while v > 8:
        goal = cart2polar(cp[0], cp[1], gp[0], gp[1])   #calculate the goal in polar coordinates, goal = [distance, angle]

        PID1 = 5                                        #The PID controller has only proportional gain to the error, controller for direction of the robot.
        v = goal[0] * PID1                              # v = Average Velacity
        v = clip(v, -100, 100)                          #saturate the velocity value from -80 to 80

        te = ThetaError(goal[1], cp[2])
        PID2 = 8                                        #The PID controller has only proportional gain to the error, controller for the velocity of the robot.
        w = te * PID2                                   # w = turning rate

        wv = WheelVelocities(v, w)                      #wv = [rigt wheel's velocity, left wheel's velocity]
  
        #write velocities to servoes
        try:
            BP.set_motor_power(BP.PORT_B, wv[0])        #right wheel velocity
            BP.set_motor_power(BP.PORT_C, wv[1])        #left wheel velocity
        except IOError as error:
            print('error, Could not write velocities')

def GetGoalPos(channel,data):
    msg = dmw_goal_position_t.decode(data)
    gp[0] = msg.x
    gp[1] = msg.y

def GetCurrentPos(channel,data):
    msg = system_state_t.decode(data)
    cp[0] = msg.x
    cp[1] = msg.y
    cp[2] = msg.theta
    drive(cp, gp)

lc = lcm.LCM()
lc.subscribe("Goal_handler", GetGoalPos)
lc.subscribe("SYSTEM_STATE", GetCurrentPos)

try:
    while True:
        lc.handle()
except KeyboardInterrupt:
    pass
