#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////
// Use together with 6-1-server.cpp to test the usage of function dup()
// ///////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
    if ( argc <= 2 ) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    // try to connect to the give addr
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    // server address
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    // create a socket
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    // you don't have to bind a addr to the socket
    // kernel will do it well

    // connect to the server address
    if ( connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ) {
        printf("connection failed!\n");
    } else {
        // connectin successful and send and recv
        char buffer[1024];
        memset(buffer, '\0', 1024);
        int recv_len = recv(sockfd, buffer, 1023, 0);
        buffer[recv_len] = '\0';
        printf("received from server %d bytes : %s", recv_len, buffer);
    }
    close(sockfd);
    return 0;
}
