#!/usr/bin/env python3
import lcm
from robot import io_arrowhead_t
import datetime, time
from flask import Flask

app = Flask(__name__)

# Publish a action through lcm
def publish_action(action, position):
    a = io_arrowhead_t()
    a.action = action
    a.pos = position
    a.timestamp = millis()
    lcm.LCM().publish("ACTION", a.encode())

def millis():
    ts = datetime.datetime.now()
    return round(time.mktime(ts.timetuple()) * 1e3
                 + ts.microsecond / 1e3)

# This function is called by a get request from the provider service pick_up, on right side
@app.route('/robot/pick_up_right', methods=['GET'])
def pick_up_right():
    publish_action('pick_up', 'right')
    return {'msg': 'Ok, picking piece at right side'}

# This function is called by a get request from the provider service pick_up, on left side
@app.route('/robot/pick_up_left', methods=['GET'])
def pick_up_left():
    publish_action('pick_up', 'left')
    return {'msg': 'Ok, picking piece at left side'}

# This function is called by a get request from the provider service place, on right side
@app.route('/robot/place_right', methods=['GET'])
def place_right():
    publish_action('place', 'right')
    return {'msg': 'Ok, placing piece at right side'}

# This function is called by a get request from the provider service place, on left side
@app.route('/robot/place_left', methods=['GET'])
def place_left():
    publish_action('place', 'left')
    return {'msg': 'Ok, placing piece at left side'}

if __name__ == '__main__':
    # Start flask
    app.run(host='127.0.0.1', port=5005)

