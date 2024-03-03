#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

// Creating a field that will represent the different types of protocol headers we can have
typedef enum {
    // A named number that is going to represent the 'hello' message in our protocol 
    PROTO_HELLO,
} proto_type_e;

// Header of our protocol
// Will have length of the protocol
// And the type of the field contained in the protocol
// What we are constructing is known as TLV (type-length-value system)
typedef struct {
    proto_type_e type;
    unsigned short len;
} proto_hdr_t;

void handle_client(int fd) {
    char buf[4096] = {0};
    proto_hdr_t *hdr = buf;
    hdr->type = htonl(PROTO_HELLO);
    hdr->len = sizeof(int);
    int reallen = hdr->len;
    hdr->len = htons(hdr->len);
    // Create a pointer to the data that is going to be put into the packet
    int *data = (int*)&hdr[1];
    *data = htonl(1);

    write(fd, hdr, sizeof(proto_hdr_t) + reallen);
}

int main() {
    struct sockaddr_in serverInfo = {0};
    struct sockaddr_in clientInfo = {0};
    socklen_t clientSize = sizeof(clientInfo);
    serverInfo.sin_family = AF_INET;
    // by setting it to 0, we can bind to every address our computer owns
    serverInfo.sin_addr.s_addr = 0;
    serverInfo.sin_port = htons(5555);


    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return -1;
    }
 
    // bind the socket to the type, port and address
    if (bind(fd, (struct sockaddr*)&serverInfo, sizeof(serverInfo)) == -1) {
        perror("bind");
        close(fd);
        return -1;
    }

    // listen
    if (listen(fd, 0) == -1) {
        perror("listen");
        close(fd);
        return -1;
    }

    // An infinite loop like this is bad practice but we are doing it for the sake of the lesson
    while (1) {
        // accept
        int clientfd = accept(fd, (struct sockaddr*)&clientInfo, &clientSize);
        if (clientfd == -1) {
            perror("accept");
            close(fd);
            return -1;
        }

        handle_client(clientfd);

        close(clientfd);
    }
}