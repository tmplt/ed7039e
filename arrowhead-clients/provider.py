import sys
from datetime import datetime
sys.path.insert(1, '/home/ruben/git/client-library-python')
import arrowhead_client.api as ar

provider_app = ar.ArrowheadHttpClient(
        system_name='lcm_provider',
        address='127.0.0.1',
        port=5000,
        keyfile='certificates/my_cloud/lcm_provider.key',
        certfile='certificates/my_cloud/lcm_provider.crt',
)

action = ''
log =''

@provider_app.provided_service(
        'post_action',
        'post_action',
        'HTTPS-SECURE-JSON',
        'POST', )
def post_action(request):
    global action
    action = request.json['action']
    print ('Action updated to:', action)
    return {"msg": "OK!"}

@provider_app.provided_service(
        'get_action',
        'get_action',
        'HTTPS-SECURE-JSON',
        'GET', )
def get_action(request):
    return {"action": action}

@provider_app.provided_service(
        'post_log',
        'post_log',
        'HTTPS-SECURE-JSON',
        'POST', )
def post_log(request):
    global log
    log = request.json['log']
    print ('Log Updated to:',log)
    return {"msg": "OK!"}

@provider_app.provided_service(
        'get_log',
        'get_log',
        'HTTPS-SECURE-JSON',
        'GET', )
def get_log(request):
    return {'log': log}

if __name__ == '__main__':
    provider_app.run_forever()
