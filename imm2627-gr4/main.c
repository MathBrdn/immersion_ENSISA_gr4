#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "network/server.h"
#include "network/joueur.h"
#include "network/network.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage :\n");
        printf("  %s -s <port>\n", argv[0]);
        printf("  %s -c <adresse> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Mode serveur */
    if (strcmp(argv[1], "-s") == 0) {

        if (argc != 3) {
            printf("Usage : %s -s <port>\n", argv[0]);
            return EXIT_FAILURE;
        }

        int port = atoi(argv[2]);

        server_start(port);
    }

    /* Mode joueur */
    else if (strcmp(argv[1], "-c") == 0) {

        if (argc != 4) {
            printf("Usage : %s -c <adresse> <port>\n", argv[0]);
            return EXIT_FAILURE;
        }

        const char *address = argv[2];
        int port = atoi(argv[3]);

        int socket = joueur_connect(address, port);

        if (socket == -1) {
            return EXIT_FAILURE;
        }

        char buffer[BUFFER_SIZE];

        if (receive_message(socket,
                            buffer,
                            sizeof(buffer)) > 0) {

            printf("Serveur : %s\n", buffer);
        }

        send_message(socket, "Bonjour serveur !");

        joueur_disconnect(socket);
    }

    else {
        printf("Option inconnue : %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}