#!/usr/bin/env python3
import lcm
from robot import action_t, log_t
import time
import sys
sys.path.insert(1, '/home/ruben/git/client-library-python') # Uses the library client-library-python
import arrowhead_client.api as ar

consumer_app = ar.ArrowheadHttpClient(
        system_name='lcm_consumer',
        address='127.0.0.1',
        port=5002,
        keyfile='/home/ruben/git/ed7039e/certificates/my_cloud/lcm_consumer.key',
        certfile='/home/ruben/git/ed7039e/certificates/my_cloud/lcm_consumer.crt',
)

consumer_app.add_consumed_service('post_log', 'POST')
consumer_app.add_consumed_service('get_action', 'GET')

action = ''
previous_action = ''

def publish_action():
    global action
    global previous_action
    if action != '' and previous_action != action:
        a = action_t()
        a.action = action
        lcm.LCM().publish("ACTION", a.encode())
    previous_action = action

def get_action():
    global previous_action
    global action
    response = consumer_app.consume_service('get_action')
    message = consumer_app.consumer.extract_payload(response, 'json')
    action = message['action']

def print_action():
    global action
    global previous_action
    if action != '' and previous_action != action:
        print ('Action published:', action)
    previous_action = action

if __name__ == '__main__':
    while True:
        get_action()
        publish_action()
        time.sleep(1)

