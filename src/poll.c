#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <string.h>
#include <poll.h>

#define MAX_CLIENTS 256
#define PORT 9128
#define BUFFER_SIZE 4096
typedef enum {
    STATE_NEW,
    STATE_CONNECTED,
    STATE_DISCONNECTED
} state_e;

typedef struct {
    int fd;
    state_e state;
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

int find_slot_by_fd(int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientStates[i].fd == fd) {
             return i;
        }
    }
    return -1;
}

int main() {
    int listen_fd, conn_fd, nfds, freeSlot;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    struct pollfd fds[MAX_CLIENTS + 1];
    nfds = 1;
    int opt = 1;

    init_clients();

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        return -1;
    }

    // make socket non waiting, short for 'set socket option'
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = 0;
    server_addr.sin_port = htons(9128);

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

    memset(fds, 0, sizeof(fds));
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    nfds = 1;

    while (1) {
        // First, rebuild the fds array for poll() on each iteration
        nfds = 1; // Start with one for listen_fd
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clientStates[i].fd != -1) {
                fds[nfds].fd = clientStates[i].fd;
                fds[nfds].events = POLLIN;
                nfds++;
            }
        }

        // Wait for an event on one of the sockets
        int n_events = poll(fds, nfds, -1); // -1 for no timeout
        if (n_events == -1) {
            perror("poll");
            break; // Exit or handle error appropriately
        }

        // Check if there's an incoming connection on the listen_fd
        if (fds[0].revents & POLLIN) {
            conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);
            if (conn_fd == -1) {
                perror("accept");
                continue; // Continue to next iteration of the loop
            }

            printf("New Connection: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            int freeSlot = find_free_slot();
            if (freeSlot == -1) {
                printf("Server is full: closing new connection\n");
                close(conn_fd);
            } else {
                clientStates[freeSlot].fd = conn_fd;
                clientStates[freeSlot].state = STATE_CONNECTED;
                // No need to increment nfds here, it's done at the beginning of the loop
                printf("Slot %d has fd %d\n", freeSlot, clientStates[freeSlot].fd);
            }
        }

        // Handle IO on other sockets
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                int slot = find_slot_by_fd(fds[i].fd);
                if (slot != -1) { // Valid slot found
                    ssize_t bytes_read = read(fds[i].fd, clientStates[slot].buffer, BUFFER_SIZE - 1);
                    if (bytes_read <= 0) {
                        // Connection closed or error
                        printf("Connection closed or error on fd %d\n", fds[i].fd);
                        close(fds[i].fd);
                        clientStates[slot].fd = -1; // Mark slot as free
                        clientStates[slot].state = STATE_DISCONNECTED;
                        // No need to decrement nfds here, it's recalculated at the start of the loop
                    } else {
                        // Data received, process here
                        // Ensure to null-terminate the received string if expecting C-strings
                        clientStates[slot].buffer[bytes_read] = '\0';
                        printf("Received data from client: %s\n", clientStates[slot].buffer);
                        // Respond to client or process data as needed
                    }
                }
            }
        }
    }
}