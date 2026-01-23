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
#include <pthread.h>
#include "server.h"
#include "http_handler.h"

#define PORT 8080
#define BUFFER_SIZE 4096

// Structure to store client info
typedef struct
{
    int client_fd;
    struct sockaddr_in client_addr;
} client_info_t;

void *handle_client(void *arg)
{
    client_info_t *client = (client_info_t *)arg;
    int client_fd = client->client_fd;
    char buffer[BUFFER_SIZE];

    printf("Thread handling connection from %s:%d\n", 
        inet_ntoa(client->client_addr.sin_addr), 
        ntohs(client->client_addr.sin_port));
        
    ssize_t bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';

        http_request_t req;
        if (http_parse_request(buffer, &req) == 0) {
            printf("%s %s\n", req.method, req.path);

            http_response_t res;
            serve_static_file(req.path, &res);

            char *response = http_build_response(&res);
            send(client_fd, response, strlen(response), 0);
            free(response);

            http_free_response(&res);
            http_free_request(&req);
        } else {
            // Bad request
            char *bad = "HTTP/1.1 400 Bad Request\r\nContent-Length: 11\r\n\r\nBad Request";
            send(client_fd, bad, strlen(bad), 0);
        }
    }
   
    close(client_fd);
    free(client); // Free allocated struct

    return NULL;
}

Server server_init(int backlog)
{
    Server server;
    server.port = PORT;

    // Create socket
    server.fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server.fd < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Allow address reuse (helps with rapid restarts)
    int opt = 1;
    if (setsockopt(server.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Configure address
    server.address.sin_family = AF_INET;
    server.address.sin_addr.s_addr = INADDR_ANY;
    server.address.sin_port = htons(server.port);

    // Bind socket to address
    if (bind(server.fd, (struct sockaddr *)&server.address, sizeof(server.address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening
    if (listen(server.fd, backlog) < 0)
    {
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

    while (1)
    {
        int client_fd = accept(server->fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        // Allocate client info for the thread (thread will free it)
        client_info_t *client = malloc(sizeof(client_info_t));
        client->client_fd = client_fd;
        client->client_addr = client_addr;

        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, client) != 0) {
            perror("pthread_create failed");
            close(client_fd);
            free(client);
            continue;
        }

        // Detach so resources are freed when thread exits
        pthread_detach(thread);
    }
}
