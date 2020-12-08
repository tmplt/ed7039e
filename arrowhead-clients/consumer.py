import time
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
        keyfile='certificates/my_cloud/consumer.key',
        certfile='certificates/my_cloud/consumer.crt',
)

# Add services to consume
consumer_app.add_consumed_service('pick_up', 'GET')
consumer_app.add_consumed_service('place', 'GET')

if __name__ == '__main__':
    
    # Consume services and print out the response message
    response = consumer_app.consume_service('pick_up')
    message = consumer_app.consumer.extract_payload(response, 'json')
    print (message['msg'])
    time.sleep(2)

    response2 = consumer_app.consume_service('place')
    message2 = consumer_app.consumer.extract_payload(response2, 'json')
    print (message2['msg'])