#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

typedef struct {
  int fd; // file descriptor for the socket
  int port; 
  struct sockaddr_in address; 
} Server;

Server server_init(int backlog);

// Main server loop
void server_launch(Server *server);

#endif