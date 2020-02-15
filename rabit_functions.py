#!/usr/bin/python3                                                                                                                                                                                        
"""
    This modules uses the pika AMQ library to implement
    rabit clients (send/consume operations)
"""
import sys
import pika


DATA_SIZE = 128
def load_data(duplicate_factor):
    """
    Python does data sizes weirdly but this should be very close to 128KiB
    """
    return 'a' * 1024 * duplicate_factor

def send_simulation(args):
    """
    Simulates a rabmq client that periodically sends data to the broker
    args (all optional) = [broker_ip , sleep_intarval, duplication factor]
    a duplication factor of 2 will result in ~ 258KB of data
    """
    import time

    broker_ip = "localhost"
    sleep_interval = 10
    duplicate_factor = 3
    #extracting args
    if args:
        broker_ip = args[0]
    if len(args) > 1:
        sleep_interval = int(args[1])
    if len(args) > 2:
        duplicate_factor = int(args[2])

    connection = pika.BlockingConnection(pika.ConnectionParameters(host=broker_ip))
    channel = connection.channel()

    channel.queue_declare(queue='hello')
    data_content = load_data(duplicate_factor)
    try:
        while True:
            channel.basic_publish(exchange='', routing_key='hello', body=data_content)
            time.sleep(sleep_interval)
            print('sending a message of size {0}K to broker id {1}'.format( len(data_content)/1000, broker_ip))
    finally:
        print("\n\nclosing the connection and exiting gracefully!\n")
        connection.close()

def rec(broker_ip='localhost'):
    """
    subscribes to the brokere on the 'hello' channel and consumes messages
    used for testing purposes
    @param broker_ip: the ip address of the broker if not localhost
    """

    print('reciving from broker %s' % broker_ip)
    connection = pika.BlockingConnection(
        pika.ConnectionParameters(host=broker_ip))

    channel = connection.channel()

    channel.queue_declare(queue='hello')


    def callback(ch, method, properties, body):
        if type(body) == str:
            print("recieved message of size ", len(body))
        else:

            print("recieved message from queue")


    channel.basic_consume(
        queue='hello', on_message_callback=callback, auto_ack=True)

    print(' [*] Waiting for messages. To exit press CTRL+C')
    channel.start_consuming()
    

if __name__ == '__main__':
    if sys.argv[1] == 'send_simulation':
        print('Running the send_simulation mode')
        send_simulation(sys.argv[2:])
    
    elif sys.argv[1] == 'rec':
        print('Running the consumption mode')
        if len(sys.argv) >= 3:
            rec(sys.argv[2])
        else:
            rec()
    else:
        print('ERROR: usage send_simulation/rec [broker_ip] [sleep_invl] [duplication_factor]. you specified: ', end='')
        print(sys.argv)