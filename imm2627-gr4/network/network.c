#include "network.h"

#include <sys/socket.h>
#include <string.h>

int send_message(int socket, const char *message)
{
    int length = strlen(message);

    return send(socket,
                message,
                length,
                0);
}

int receive_message(int socket,
                    char *buffer,
                    int buffer_size)
{
    int received;

    received = recv(socket,
                    buffer,
                    buffer_size - 1,
                    0);

    if (received > 0) {
        buffer[received] = '\0';
    }

    return received;
}