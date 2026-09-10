#ifndef JOUEUR_H
#define JOUEUR_H

int joueur_connect(const char *address, int port);
void joueur_disconnect(int client_socket);

#endif