#ifndef NETWORK_H
#define NETWORK_H

#define BUFFER_SIZE 1024

int send_message(int socket, const char *message);

int receive_message(int socket,
                    char *buffer,
                    int buffer_size);

#endif