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
    read(fd, buf, sizeof(proto_hdr_t) + sizeof(int));
    
    proto_hdr_t *hdr = buf;
    hdr->type = ntohl(hdr->type);
    hdr->len = ntohs(hdr->len);

    int *data = &hdr[1];
    *data = ntohl(data);

    if (hdr->type != PROTO_HELLO) {
        printf("Protocol mismatch, failing.");
        return;
    }

    if (*data != 1) {
        printf("Protocol version mismatch, failing.");
        return;
    }

    printf("Server connected, protocol v1.\n");
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usag: %s <ip of the host>\n", argv[0]);
        return 0;
    }

    struct sockaddr_in serverInfo = {0}; 

    serverInfo.sin_family = AF_INET;
    // Specify the server IP address to connect to, will take an argument from the cli
    serverInfo.sin_addr.s_addr = inet_addr(argv[1]);
    serverInfo.sin_port = htons(5555);


    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return -1;
    }

    if(connect(fd, (struct sockaddr*)&serverInfo, sizeof(serverInfo)) == -1) {
        perror("connect");
        close(fd);
        return -1;
    }
}