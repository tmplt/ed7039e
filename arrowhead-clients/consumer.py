import urllib3
import sys
sys.path.insert(1, '/home/ruben/git/client-library-python')
import arrowhead_client.api as ar

urllib3.disable_warnings()

# Creating the consumer_app
consumer_app = ar.ArrowheadHttpClient(
        system_name='consumer',
        address='127.0.0.1',
        port=5001,
        keyfile='certificates/consumer.key',
        certfile='certificates/consumer.crt',
)

# Add services to consume
consumer_app.add_consumed_service('pick_up', 'POST')
consumer_app.add_consumed_service('place', 'POST')

if __name__ == '__main__':
    
    while True:
        # Consume services and print out the response message
        print('Enter service:')
        x = input()
        if x == 'pick_up l' or x == 'pick up l':
            response = consumer_app.consume_service('pick_up', json={'position': 'loading'})
            message = consumer_app.consumer.extract_payload(response, 'json')
            print (message['msg'])
        if x == 'place l':
            response2 = consumer_app.consume_service('place', json={'position': 'loading'})
            message2 = consumer_app.consumer.extract_payload(response2, 'json')
            print (message2['msg'])
        if x == 'pick_up u' or x == 'pick up u':
            response = consumer_app.consume_service('pick_up', json={'position': 'unloading'})
            message = consumer_app.consumer.extract_payload(response, 'json')
            print (message['msg'])
        if x == 'place u':
            response2 = consumer_app.consume_service('place', json={'position': 'unloading'})
            message2 = consumer_app.consumer.extract_payload(response2, 'json')
            print (message2['msg'])
        