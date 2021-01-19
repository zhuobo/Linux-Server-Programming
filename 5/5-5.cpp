#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

// choose a socket from the socket listening queue to accept
int main(int argc, char* argv[]) {
    if ( argc <= 2 ) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    // socket address
    struct sockaddr_in address; // define a ipv4 address  family
    bzero(&address, sizeof(address)); //set all to zero
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    // create a socket
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    // bind the socket with socket address input by user
    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    // listen to the socket 
    ret = listen( sock, 5 );
    assert( ret != -1 );

    // wait for connect 
    sleep(60);

    // accept a socket is being listen
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof( client );
    // return a socket fd
    int connfd = accept( sock, ( struct sockaddr* )&client, &client_addrlength );
    if ( connfd < 0 ) {
        printf( "errno is : %d", errno );
    }
    else {
        char remote[INET_ADDRSTRLEN];
        printf(" connected with ip: %s and port : %d\n", inet_ntop(AF_INET, &client.sin_addr, remote, INET_ADDRSTRLEN), ntohs(client.sin_port));
        close(connfd);
    }
    close(sock);
    return 0;
}
