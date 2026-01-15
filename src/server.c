// Server's job is to:
// 1. Open Sockets
// 2. Bind to a port and listen
// 3. Accept Connections
// 4. Read/write to the socket
// 5. Close the socket.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "server.h"

#define PORT 8080

Server server_init(int backlog)
{
    Server server;
    server.port = PORT;

    // Create socket
    server.fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server.fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Allow address reuse (helps with rapid restarts)
    int opt = 1;
    if (setsockopt(server.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Configure address
    server.address.sin_family = AF_INET;
    server.address.sin_addr.s_addr = INADDR_ANY;
    server.address.sin_port = htons(server.port);

    // Bind socket to address
    if (bind(server.fd, (struct sockaddr *)&server.address, sizeof(server.address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening
    if (listen(server.fd, backlog) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", server.port);

    return server;
}

void server_launch(Server *server)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (1) {
        int client_fd = accept(server->fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        printf("Connection accepted from %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        // TODO: Handle the connection (read request, send response)

        close(client_fd);
    }
}
