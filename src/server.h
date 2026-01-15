#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

typedef struct {
  int fd;
  int port;
  struct sockaddr_in address;
} Server;

Server server_init(int domain, int service, int protocol, unsigned long int interface, int port, int backlog);

// Main server loop
void server_launch(Server *server);

#endif