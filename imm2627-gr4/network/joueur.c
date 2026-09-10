#include "joueur.h"
#include "network.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int joueur_connect(const char *address, int port)
{
    int joueur_socket;

    struct sockaddr_in server_addr;

    /* Création de la socket */
    joueur_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (joueur_socket == -1) {
        perror("socket");
        return -1;
    }

    /* Configuration de l'adresse du serveur */
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET,
                  address,
                  &server_addr.sin_addr) <= 0) {

        perror("inet_pton");
        close(joueur_socket);
        return -1;
    }

    /* Connexion au serveur */
    if (connect(joueur_socket,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) == -1) {

        perror("connect");
        close(joueur_socket);
        return -1;
    }

    printf("Connexion au serveur réussie.\n");

    return joueur_socket;
}

void joueur_disconnect(int joueur_socket)
{
    if (joueur_socket >= 0) {
        close(joueur_socket);
    }
}