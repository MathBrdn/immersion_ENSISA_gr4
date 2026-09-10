#include "server.h"
#include "network.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>

int server_start(int port)
{
    int server_socket;
    int player1_socket;
    int player2_socket;

    struct sockaddr_in server_addr;

    /* Création de la socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == -1) {
        perror("socket");
        return -1;
    }

    /* Autorise la réutilisation du port */
    int option = 1;

    if (setsockopt(server_socket,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &option,
                   sizeof(option)) == -1) {
        perror("setsockopt");
        close(server_socket);
        return -1;
    }

    /* Configuration de l'adresse du serveur */
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Association IP + port */
    if (bind(server_socket,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_socket);
        return -1;
    }

    /* Mise en écoute */
    if (listen(server_socket, 2) == -1) {
        perror("listen");
        close(server_socket);
        return -1;
    }

    printf("Serveur en écoute sur le port %d...\n", port);

    /* Joueur 1 */
    printf("Attente du joueur 1...\n");

    player1_socket = accept(server_socket, NULL, NULL);

    if (player1_socket == -1) {
        perror("accept");
        close(server_socket);
        return -1;
    }

    printf("Joueur 1 connecté.\n");

    /* Joueur 2 */
    printf("Attente du joueur 2...\n");

    player2_socket = accept(server_socket, NULL, NULL);

    if (player2_socket == -1) {
        perror("accept");
        close(player1_socket);
        close(server_socket);
        return -1;
    }

    printf("Joueur 2 connecté.\n");

    /*
     * Pour l'instant, simple test de communication.
     */

    send_message(player1_socket, "Bienvenue joueur 1 !");
    send_message(player2_socket, "Bienvenue joueur 2 !");

    char buffer[BUFFER_SIZE];

    int result = receive_message(player1_socket,
                                 buffer,
                                 sizeof(buffer));

    if (result > 0) {
        printf("Joueur 1 : %s\n", buffer);

        send_message(player2_socket, buffer);
    }

    /* Fermeture des connexions */
    close(player1_socket);
    close(player2_socket);
    close(server_socket);

    return 0;
}

void server_stop(int server_socket)
{
    if (server_socket >= 0) {
        close(server_socket);
    }
}