#!/usr/bin/env python3
import lcm
from robot import action_t
import time
from flask import Flask

app = Flask(__name__)

# Publish a action through lcm
def publish_action(action):
    a = action_t()
    a.action = action
    lcm.LCM().publish("ACTION", a.encode())

# This function is called by a get request from the provider service pick_up,
# which will then publish the message: 'pick_up'
@app.route('/robot/pick_up')
def pick_up():
    publish_action('pick_up')
    return {'msg': 'OK, picking up!'}

# This function is called by a get request from the provider service place,
# which will then publish the message: 'place'
@app.route('/robot/place')
def place():
    publish_action('place')
    return {'msg': 'Ok, placing!'}

if __name__ == '__main__':
    # Start flask
    app.run(host='127.0.0.1', port=5005)

