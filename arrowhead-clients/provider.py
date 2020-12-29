#!/usr/bin/env python3
import sys
from sshtunnel import SSHTunnelForwarder
import requests
sys.path.insert(1, '/home/ruben/git/client-library-python')
import arrowhead_client.api as ar

remote_host = 'tmplt.dev'                    # ip to raspberry pi
remote_port = 21013                          # port for ssh
local_host = '127.0.0.1'                     # For the communication with lcm-arrowhead
local_port = 5005                            # For the communication with lcm-arrowhead

# Creating ssh tunnel for the communication between robot and server
server = SSHTunnelForwarder(
   (remote_host, remote_port),
   ssh_username='root',
   ssh_private_key='/home/ruben/.ssh/id_rsa',
   remote_bind_address=(local_host, local_port),
   local_bind_address=(local_host, local_port),
   )

# Creating Provider app which will be listening on request from consumer through arrowhead
# The ip and port are registred in arrowhead, which the consumer will later use to reach the provider
provider_app = ar.ArrowheadHttpClient(
        system_name='lcm_provider',
        address='127.0.0.1',                
        port=5000,
        keyfile='certificates/provider.key',
        certfile='certificates/provider.crt',
)

# A service pick_up which will notify the robot to pick up a piece and where
@provider_app.provided_service('pick_up', 'pick_up', 'HTTPS-SECURE-JSON', 'POST', )
def pick_up(request):
    if not 'position' in request.json:
        print ('FAULT')
        return {"msg": "Wrong input"}
    position = str(request.json['position'])
    if position == 'loading':
        url = 'http://{}:{}/robot/pick_up_left'.format(local_host, local_port)
    elif position == 'unloading':
        url = 'http://{}:{}/robot/pick_up_right'.format(local_host, local_port)
    else:
        return {"msg": "Wrong input"}
    response = requests.get(url).json()
    return response

# A service place which will notify the robot to place a piece and where
@provider_app.provided_service('place', 'place', 'HTTPS-SECURE-JSON', 'POST', )
def place(request):
    if not 'position' in request.json:
        print ('FAULT')
        return {"msg": "Wrong input"}
    position = str(request.json['position'])
    if position == 'loading':
        url = 'http://{}:{}/robot/place_left'.format(local_host, local_port)
    elif position == 'unloading':
        url = 'http://{}:{}/robot/place_right'.format(local_host, local_port)
    else:
        return {"msg": "Wrong input"}
    response = requests.get(url).json()
    return response

if __name__ == '__main__':
    # Starting ssh tunnel
    server.start()
    # Running the provider app
    provider_app.run_forever()