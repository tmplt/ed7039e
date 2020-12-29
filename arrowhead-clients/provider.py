''' This will be running on a server '''
import sys
from datetime import datetime
import time
from sshtunnel import SSHTunnelForwarder
import requests
# Path to the client-library-python which the provider is using
# https://github.com/arrowhead-f/client-library-python
sys.path.insert(1, '/home/ruben/git/client-library-python')
import arrowhead_client.api as ar

remote_host = ''                    # ip to the raspberry pi
remote_port = 22                          # port for ssh
local_host = '127.0.0.1'
local_port = 5005                       

# Creating ssh tunnel for the communication between robot and server
server = SSHTunnelForwarder(
   (remote_host, remote_port),
   ssh_username='root',
   ssh_private_key='/home/ruben/.ssh/id_rsa.pub',
   remote_bind_address=(local_host, local_port),
   local_bind_address=(local_host, local_port),
   )


# Creating Provider app which will be running on the server along arrowhead core systems
provider_app = ar.ArrowheadHttpClient(
        system_name='lcm_provider',
        address='127.0.0.1',
        port=5000,
        keyfile='certificates/provider.key',
        certfile='certificates/provider.crt',
)

# A service pick_up which will notify the robot to pick up a piece
@provider_app.provided_service('pick_up', 'pick_up', 'HTTPS-SECURE-JSON', 'GET', )
def pick_up(request):
    url = 'http://{}:{}/robot/pick_up'.format(local_host, local_port)
    response = requests.get(url).json()
    return response

# A service place which will notify the robot to place a piece
@provider_app.provided_service('place', 'place', 'HTTPS-SECURE-JSON', 'GET', )
def place(request):
    url = 'http://{}:{}/robot/place'.format(local_host, local_port)
    response = requests.get(url).json()
    return response

if __name__ == '__main__':
    # Starting ssh tunnel
    server.start()
    # Running the provider app
    provider_app.run_forever()