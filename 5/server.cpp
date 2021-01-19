#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>

#define MAXLINE 1024

void reverse(char* str, int len);

int main(int argc, char* argv[]) {
    if ( argc <= 2 ) {
        printf("usage: %s host_ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    // create a socket
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    printf("by socket(): %d\n", socketfd);
    assert(socketfd >= 0);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    // bind ip and port to the socket
    int ret = bind(socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    assert(ret != -1);

    // listen to the socket
    ret = listen(socketfd, 5);
    
    printf("Accepting connections from clients...\n");
    
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buff[MAXLINE];

    while ( true ) {
        // accept a connection
        int connfd = accept(socketfd, (struct sockaddr*)&client_addr, &client_addr_len);
        printf("by accept(): %d\n", connfd); 
        int n = read(connfd, buff, MAXLINE - 1);
        printf("Received %d bytes from client: %s\n", n, buff);
        reverse(buff, n);
        printf("Response data : %s\n", buff);
        write(connfd, buff, n);
        close(connfd);
    }

    return 0;
}


void reverse(char* str, int len) {
    int i = 0, j = len - 1;
    while ( i < j ) {
        char c = str[i];
        str[i] = str[j];
        str[j] = c;
        ++i;
        --j;
    }
}
