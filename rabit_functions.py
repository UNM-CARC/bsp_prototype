#!/usr/bin/python3                                                                                                                                                                                        
 
import pika
import sys
"""                                                                                                                                                                                                       
Args (The args are passed by the shell script (ex bash runrabit.sh send_simulation localhost 5 4):                                                                                                        
server_add: The ip address of rabbitmq broker                                                                                                                                                           
sleep_interval (optional default = 10): how often should the client send the data                                                                                                                       
duplicate_factor (optional default=3): the initial file is ~14MB large, if we                                                                                                                           
wanna experiment with a larger message we can use this argument                                                                                                                                       
A simple script to simulate rabbitmq client that wakes every sleep_interval seconds and sends  a message to the server                                                                                                                                                                                 
"""

DATA_SIZE=128
def load_data(duplicate_factor):
    return 'a' * 1024 * duplicate_factor

def send_simulation(args):
    import time

    broker_ip = "localhost"
    sleep_interval = 10
    duplicate_factor = 3
    if len(args) > 0:
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

def rec(args):
    broker_ip = "localhost"
    if len(args) > 0:
        broker_ip = args[0]

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
        rec(sys.argv[2:])
    else:
        print('ERROR: usage send_simulation/rec ip port. you specified: ', end='')
        print(sys.argv)