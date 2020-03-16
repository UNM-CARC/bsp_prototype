#ifndef AMQ_PRODUC_H
#define AMQ_PRODUC_H

#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <amqp_framing.h>

  
int setupAmq(amqp_connection_state_t *conn, char const *hostname, int port);
int sendBatch(amqp_connection_state_t conn, char const *queue_name, int message_count, int message_size, char* message, unsigned char verbose);
int shutdownAmpq(amqp_connection_state_t conn);

#endif
