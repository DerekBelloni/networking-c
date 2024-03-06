#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <string.h>

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
    int listen_fd, conn_fd, nfds, freeSlot;
    struct sockaddr_in server_addr, client_addr;
    fd_set read_fds, write_fds;

    init_clients();

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        return -1;
    }

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
    
    while(1) {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        FD_SET(listen_fd, &read_fds);
        nfds = listen_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clientStates[i].fd != -1) {
                FD_SET(clientStates[i].fd, &read_fds);
                if (clientStates[i].fd > nfds) nfds = clientStates[i].fd;
            }
        }

        nfds += 1;

        if (select(nfds, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            return -1;
        }

        socklen_t client_addr_len = sizeof(client_addr);

        if (FD_ISSET(listen_fd, &read_fds)) {
            conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_addr_len);
            if (conn_fd == -1) {
                perror("accept");
                continue;
            }

            printf("New Connection: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            freeSlot = find_free_slot();
            if (freeSlot == -1) {
                printf("Server is full: closing new connection\n");
                close(conn_fd);
            } else {
                clientStates[freeSlot].fd = conn_fd;
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clientStates[i].fd != -1 && FD_ISSET(clientStates[i].fd, &read_fds)) {
                ssize_t bytes_read = read(clientStates[i].fd, clientStates[i].buffer, BUFFER_SIZE - 1);

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