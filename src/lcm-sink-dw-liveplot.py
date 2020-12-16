#!/usr/bin/env python3
from matplotlib import pyplot as plt
from matplotlib import gridspec as gridspec
import lcm
import numpy as np

from robot import kalman_position_t, io_position_t


class Liveplot(object):
    """
    Liveplot of position estimation from decawave data
    """
    def __init__(self, duration=100):
        self.x = 0
        self.xe = 0
        self.y = 0
        self.ye = 0
        self.counter = 0
        self.duration = duration

    def live_plot(self,pause_time=0.1):

        # position-est in xy-plane
        ax1.plot(self.xe, self.ye, '.', color='r')
        ax1.set_xlabel('x')
        ax1.set_ylabel('y')        

        # x-axis over time, measurement '*' and estimation '.'
        ax2.plot(self.counter, self.x, '*', color='b')
        ax2.plot(self.counter, self.xe, '.', color='r')
        ax2.set_ylabel('x')

        # y-axis over time, measurement '*' and estimation '.'
        ax3.plot(self.counter, self.y, '*', color='b')
        ax3.plot(self.counter, self.ye, '.', color='r')
        ax3.set_ylabel('y')

        self.counter += 1

        # pause to plot and the continue script
        plt.pause(0.001)

    def pos_handler(self, channel, data):
        msg = io_position_t.decode(data)
        self.x = msg.x
        self.y = msg.y

        if self.counter < self.duration:
            self.live_plot()
        else:
            exit()
        

    def est_handler(self, channel, data):
        msg = kalman_position_t.decode(data)
        self.xe = msg.x
        self.ye = msg.y

        print("x: {:.3f} x': {:.3f} y: {:.3f} y': {:.3f}".format(
            self.x, self.xe, self.y, self.ye))




if __name__ == "__main__":
    lc = lcm.LCM()
    lp = Liveplot()

    # plot initializing
    # -----------------
    plt.style.use('bmh')
    plt.ion()
    gs = gridspec.GridSpec(3,2)
    #fig, (ax1, ax2, ax3) = plt.subplots(nrows=3,ncols=1)
    plt.figure()
    ax1 = plt.subplot(gs[1:, :])
    ax2 = plt.subplot(gs[0,0])
    ax3 = plt.subplot(gs[0,1])

    ax1.set_ylim(0,5)
    ax1.set_xlim(0,5)
    ax2.set_ylim(0,5)
    ax3.set_ylim(0,5)
    # -----------------

    lc.subscribe("KALMAN_POSITION", lp.est_handler)
    lc.subscribe("IO_POSITION", lp.pos_handler)

    try:
        while True:
            lc.handle()
    except KeyboardInterrupt:
        pass