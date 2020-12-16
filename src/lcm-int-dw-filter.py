#!/usr/bin/env python3
import lcm 
import numpy as np
from scipy import linalg
from robot import io_position_t, kalman_position_t, io_acceleration_t

class Kalman(object):
    """
    Kalmanfilter for Decawave position data(uwb).
    using velocity from encoder node (vx, vy)

    On statespace model

    x[k+1] = Ax[k] + Bu[k] + w[k]
         y = Cx[k] + v[k]

    """



    def __init__(self):
        self.dt = 0.1   # sampling time (should prob use dw timestamp)

        self.xhat = np.transpose(np.array([[4., 0., 2., 0.]]))  # state est 
        self.yt = np.transpose(np.array([[0., 0., 0., 0.]]))    # residual
        self.u = np.transpose(np.array([[0., 0.]]))             # input

        self.z = np.transpose(np.array([[0., 0.]])) # dw measurement 

        # Statespace matrices
        self.A = np.array([[1., self.dt, 0., 0.],
                            [0., 1., 0., 0],
                            [0., 0., 1., self.dt],
                            [0., 0., 0., 1.]])
        
        self.B = np.array([[0, 0],
                            [self.dt, 0],
                            [0, 0],
                            [0, self.dt]])

        self.AT = np.transpose(self.A)

        self.C = np.array([[1., 0., 0., 0.],
                            [0., 0., 1., 0]])

        self.CT = np.transpose(self.C)

        # Kalman matrices        
        self.Q = np.array([[10., 0., 1., 0.],
                            [0., 0.1, 0., 0],
                            [1., 0., 1., 0.],
                            [0., 0., 0., 0.1]])
                    
        self.R = np.array([[10000., 100.], 
                            [100., 10000.]])

        self.S = np.array([[0., 0.], 
                            [0., 0.]])

        # Riccatti
        self.P = linalg.solve_discrete_are(a=np.transpose(self.A), 
                b=np.transpose(self.C), q=self.Q, r=self.R)


    def predict(self):

        self.xhat = np.matmul(self.A, self.xhat) #+ np.matmul(self.B, self.u)
        self.P = np.matmul(self.A, np.matmul(self.P, self.AT)) + self.Q


    def update(self):
        self.yt = self.z - np.matmul(self.C, self.xhat)

        self.S = np.matmul(self.C, np.matmul(self.P, self.CT)) + self.R
        Sinv = linalg.inv(self.S)
        self.K = np.matmul(self.P, np.matmul(self.CT, Sinv))

        self.xhat = self.xhat + np.matmul(self.K, self.yt)
        
        self.P = np.matmul(np.eye(4) - np.matmul(self.K, self.C), self.P)

        self.yhat = self.z - np.matmul(self.C,self.xhat)

    def kalman_filter(self):
        self.predict()
        self.update()

        msg = kalman_position_t()
        msg.x = self.xhat[0]
        msg.y = self.xhat[2]
        
        lc.publish("KALMAN_POSITION", msg.encode())

    def pos_handler(self,channel, data):
        msg = io_position_t.decode(data)
        self.z[0] = msg.x
        self.z[1] = msg.y 

        self.kalman_filter()

        #print("x: {:.3f} x': {:.3f} y: {:.3f} y': {:.3f}".format(
        #    self.z[0][0], self.xhat[0][0], self.z[1][0], self.xhat[2][0]))

    def acc_handler(self, channel, data):
        msg = io_acceleration_t.decode(data)
        self.u[0] = msg.x
        self.u[1] = msg.y


if __name__ == "__main__":
    lc = lcm.LCM()
    kal = Kalman()

    lc.subscribe("IO_POSITION", kal.pos_handler)
    #lc.subscribe("IO_ACCELERATION", kal.acc_handler)

    try:
        while True:
            lc.handle()
    except KeyboardInterrupt:
        pass
