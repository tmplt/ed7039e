from robot import init_pos_t, dmw_position_t, encoder_data_t
from __future__ import print_function # use python 3 syntax but make it compatible with python 2
from __future__ import division       # 
from numpy import clip

import math
import brickpi3 # import the BrickPi3 drivers
import time

BP = brickpi3.BrickPi3()    # Create an instance of the BrickPi3 class. BP will be the BrickPi3 object.

def EncoderPlant(position,TickR,TickL,Time):
  '''
  In here a new position is  estimated with the nummber of encoder ticks.
  x-axis & y-axis velocities are estimated 
  position = [x, y, theta]
  Tick(R/L) = number of encoder tick respective servo R/L
  '''
    l = 21                      #the width of the robot in [cm] 
    r = 1.52                    #The radius of the tier [cm] = 1.52
    n = 360                     #number of ticks for one revolution

    Dr = 2*math.pi*r*TickR/n    # The distance right wheel has traveled in [cm]
    Dl = 2*math.pi*r*TickL/n    # The distance left wheel has traveled in [cm]
    Dc = (Dr + Dl)/2            # The average distance body has traveled in [cm]
    #print(Dr,Dl,Dc)
    NewX = position[0] + math.cos(position[2])*Dc   # new x-position
    NewY = position[1] + math.sin(position[2])*Dc   # new y-position
    NewTheta = position[2] + (Dr - Dl)/l            # new Theta
    velX = (math.cos(position[2])*Dc) / Time        # x-axis velocity
    velY = (math.sin(position[2])*Dc) / Time        # y-axis velocity
return [NewX, NewY, NewTheta, velX, velY]

def EncoderFeedback(cp):
    BP.offset_motor_encoder(BP.PORT_A, BP.get_motor_encoder(BP.PORT_A)) # reset encoder A
    BP.offset_motor_encoder(BP.PORT_B, BP.get_motor_encoder(BP.PORT_B)) # reset encoder B   

    tic = time.time()                           #tic-toc for calculation of elapsed time

    TickR = 0
    TickL = 0

    '''
    in here we need to get positions [x, y, Theta]
    we asuume that we know current position  cp = [x, y, theta]
    '''

    time.sleep(0.1)                             #waiting 0.1 sec. this is how we change sampling frequency

    try: 
        TickR = BP.get_motor_encoder(BP.PORT_A) #TickR = number of ticks for right servo
        TickL = BP.get_motor_encoder(BP.PORT_B) #TickL = number of ticks for left servo during the time, theese are the diffrance 
    except IOError as error:
    print('error, Could not read encoders data')

    toc = time.time() 
    Time = tic - toc
    
    temp = EncoderPlant(cp, TickR, TickL, Time)
    cp = [temp[0], temp[1], temp[2]]            # New position, update the current position
    velX = temp[3]
    velY = temp[4]
return [np, velX, velY, Time]

cp = [0,0,0]                                    # current position (x, y, theta); Global variable;
initial_enable = False

def GetInitPos(channel,data):
    msg = init_pos_t.decode(data)
    initial_enable = msg.enable
    if initial_enable:
        cp[0] = msg.x
        cp[1] = msg.y
        cp[2] = msg.rad
    else:
        pass

def GetKalmanPos(channel,data):
    if not initial_enable:
        msg = dmw_position_t.decode(data)
        cp[0] = msg.x
        cp[1] = msg.y
    else:
        pass

def EncoderData(channel,data):
    data = EncoderFeedback(cp)
    msg = encoder_data_t()
    msg.x = data][0][0]
    msg.y = data][0][1]
    msg.rad = data][0][2]
    msg.vel_x = data[1]
    msg.vel_y = data[2]
    msg. duration_time = data[3]

    msg.encode()

lc = lcm.LCM()
lc.subscribe("Init_handler", GetInitPos)
lc.subscribe("Kalman_filter", GetKalmanPos)
lc.publish("encoder_handler", EncoderData)

try:
    while True:
        lc.handle()
except KeyboardInterrupt:
    pass
