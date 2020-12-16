from robot import init_pos_t, dmw_position_t, encoder_data_t
from __future__ import print_function # use python 3 syntax but make it compatible with python 2
from __future__ import division       # 

import motor-conf
import lcm
import math
import brickpi3                 # import the BrickPi3 drivers
import time

BP = motor-conf.BOT_BP          # Create an instance of the BrickPi3 class. BP will be the BrickPi3 object.

def EncoderPlant(position,TickR,TickL,Time):
  '''
  In here a new position is  estimated with the nummber of encoder ticks.
  x-axis & y-axis velocities are estimated 
  position = [x, y, theta]
  Tick(R/L) = number of encoder tick respective servo R/L
  '''
    l = 0.21                      #the width of the robot in [m] 
    r = 0.0152                    #The radius of the tier in [m]; in [cm] = 1.52
    n = 360                     #number of ticks for one revolution

    Dr = 2*math.pi*r*TickR/n    # The distance right wheel has traveled in [m]
    Dl = 2*math.pi*r*TickL/n    # The distance left wheel has traveled in [m]
    Dc = (Dr + Dl)/2            # The average distance body has traveled in [m]
    #print(Dr,Dl,Dc)
    NewX = position[0] + math.cos(position[2])*Dc   # new x-position
    NewY = position[1] + math.sin(position[2])*Dc   # new y-position
    NewTheta = position[2] + (Dr - Dl)/l            # new Theta
    dX = (math.cos(position[2])*Dc) / Time          # x-axis velocity
    dY = (math.sin(position[2])*Dc) / Time          # y-axis velocity
    dTheta = (NewTheta - position[2]) / Time        # the angular velovity of the robot
return [NewX, NewY, NewTheta, dX, dY, dTheta]

TickR = 0
TickL = 0

def EncoderFeedback(cp):
    BP.offset_motor_encoder(BP.PORT_B, BP.get_motor_encoder(BP.PORT_B)) # reset encoder B 
    BP.offset_motor_encoder(BP.PORT_C, BP.get_motor_encoder(BP.PORT_C)) # reset encoder C 

    tic = time.time()                           #tic-toc for calculation of elapsed time
    '''
    in here we need to get positions [x, y, Theta]
    we asuume that we know current position  cp = [x, y, theta]
    '''

    time.sleep(0.1)                             #waiting 0.1 sec. this is how we change sampling frequency

    try: 
        TickR = BP.get_motor_encoder(BP.PORT_B) #TickR = number of ticks for right servo
        TickL = BP.get_motor_encoder(BP.PORT_C) #TickL = number of ticks for left servo during the time, theese are the diffrance 
    except IOError as error:
    print('error, Could not read encoders data')

    toc = time.time() 
    Time = tic - toc
    
    temp = EncoderPlant(cp, TickR, TickL, Time)
    cp = [temp[0], temp[1], temp[2]]            # New position, update the current position
    dX = temp[3]
    dY = temp[4]
    dTheta = temp[5]
return [cp, dX, dY, dTheta, Time]

cp = [0,0,0]                                    # current position (x, y, theta); Global variable;
#initial_enable = False
lc = lcm.LCM()

def EncoderData():
    data = EncoderFeedback(cp)
    msg = system_state_t()
    msg.x = data[0][0]
    msg.y = data[0][1]
    msg.theta = data[0][2]
    msg.dx = data[1]
    msg.dy = data[2]
    msg.dtheta = data[3]
    msg.dt= data[4]
    return msg.encode()

def GetInitPos(channel,data):
    msg = init_pos_t.decode(data)               ###### this node is not complitet, need to change channle name wen its done
    cp[0] = msg.x
    cp[1] = msg.y
    cp[2] = msg.theta
    lc.publish("SYSTEM_STATE", EncoderData())

def GetKalmanPos(channel,data):
    msg = kalman_position_t.decode(data)
    cp[0] = msg.x
    cp[1] = msg.y
    lc.publish("SYSTEM_STATE", EncoderData())

lc.subscribe("Init_handler", GetInitPos)
lc.subscribe("KALMAN_POSITION", GetKalmanPos)

try:
    while True:
        lc.handle()
except KeyboardInterrupt:
    pass
