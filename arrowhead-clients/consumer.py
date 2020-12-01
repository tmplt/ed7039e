import time
import urllib3
import sys
sys.path.insert(1, '/home/ruben/git/client-library-python')
import arrowhead_client.api as ar

urllib3.disable_warnings()

consumer_app = ar.ArrowheadHttpClient(
        system_name='consumer',
        address='127.0.0.1',
        port=5001,
        keyfile='certificates/consumer.key',
        certfile='certificates/consumer.crt',
)

consumer_app.add_consumed_service('post_action', 'POST')
consumer_app.add_consumed_service('get_log', 'GET')

if __name__ == '__main__':

    response = consumer_app.consume_service('post_action', json={'action': 'pick_up_piece'})
    message = consumer_app.consumer.extract_payload(response, 'json')
    print (message['msg'])
    time.sleep(3)
    response = consumer_app.consume_service('post_action', json={'action': 'drop_off_piece'})
    time.sleep(3)
    response = consumer_app.consume_service('post_action', json={'action': ''})
    time.sleep(3)
    response = consumer_app.consume_service('post_action', json={'action': 'stop'})
'''
    for i in range(0,4):
        response3 = consumer_app.consume_service('get_log')
        message3 = consumer_app.consumer.extract_payload(response3, 'json')
        print (message3['log'])
        time.sleep(1)
'''
