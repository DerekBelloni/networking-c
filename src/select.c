#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <string.h>
// I want to write a simple program using select to allow two clients to connect at the same time
// I will use netcat for connecting as client
// I need a a global struct for the client state
// I dont think I need a state machine for this, just an fd and a buffer
// I need to define max clients, port, buffer size
#define MAX_CLIENTS 2
#define PORT 9129
#define BUFFER_SIZE 4096

typedef struct {
    int fd;
    char buffer[4096];
} clientstate_t;

clientstate_t clientStates[MAX_CLIENTS];

void init_clients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clientStates[i].fd = -1;
        memset(&clientStates[i].buffer, '\0', BUFFER_SIZE);
    }
};

int find_free_slot() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientStates[i].fd == -1) {
            return i;
        }
    }
    return -1;
};

int main() {
    // initialize variables for fd's
    int listen_fd, conn_fd, nfds, freeSlot;
    // initialize variables for socket structs
    struct sockaddr_in server_addr, client_addr;
    // initialize variables for fd sets
    fd_set read_fds, write_fds;

    // initialize the clients
    init_clients();

    // create listening socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        return -1;
    }

    // set up server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = 0;
    server_addr.sin_port = htons(9129);

    // bind the socket
    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(listen_fd);
        return -1;
    }

    // listen to the socket
    if (listen(listen_fd, 0) == -1) {
        perror("listen");
        close(listen_fd);
        return -1;
    }

    printf("Server listening on port %d\n", PORT);
    
    // Handle select() functionality
    while(1) {
        FD_ZERO(&read_fds);
        // FD_ZERO(&write_fds); // If you're not using write_fds yet, this can be omitted

        FD_SET(listen_fd, &read_fds);
        nfds = listen_fd;

        // loop through clientStates and add active connections to read set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clientStates[i].fd != -1) {
                FD_SET(clientStates[i].fd, &read_fds);
                if (clientStates[i].fd > nfds) nfds = clientStates[i].fd;
            }
        }

        nfds += 1; // nfds should be 1 more than the highest file descriptor number

        // Wait for activity on one of the sockets
        if (select(nfds, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            return -1;
        }

        socklen_t client_addr_len = sizeof(client_addr);
        // Check for new connections on the listening socket
        if (FD_ISSET(listen_fd, &read_fds)) {
            conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_addr_len);
            if (conn_fd == -1) {
                perror("accept");
                continue;
            }

            printf("New Connection: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            // Find a free slot for our new connection
            freeSlot = find_free_slot();
            if (freeSlot == -1) {
                printf("Server is full: closing new connection\n");
                close(conn_fd);
            } else {
                clientStates[freeSlot].fd = conn_fd;
            }
        }

        // Now handle every other client inside the server
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clientStates[i].fd != -1 && FD_ISSET(clientStates[i].fd, &read_fds)) {
                printf("Checking client %d\n", i);
                ssize_t bytes_read = read(clientStates[i].fd, clientStates[i].buffer, BUFFER_SIZE - 1);

                // if bytes_read is <= 0, close the fd and set clientStates[i].fd = -1
                if (bytes_read <= 0) {
                    close(clientStates[i].fd);
                    clientStates[i].fd = -1;
                    memset(&clientStates[i].buffer, '\0', BUFFER_SIZE);
                    printf("Client disconnected or error\n");
                } else {
                    clientStates[i].buffer[bytes_read] = '\0';
                    printf("Received data from client %d: %s\n", i, clientStates[i].buffer);
                }
            } 
        }
    }

}