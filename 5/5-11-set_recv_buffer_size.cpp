#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define BUFF_SIZE 1024

int main(int argc, char* argv[]) {
    if ( argc <= 2 ) {
        printf("usage: %s ip_address port_number recv_buffer_size\n",
                basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    
    // create a address by arguments ip and port number
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);


    // create a socket
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);
    
    int recvbuf = atoi(argv[3]);
    int len = sizeof(recvbuf);

    // set TCP receive buffer size and the read the real size after set
    // eg. I set receive buffer size 50 and it became 2304 finally
    // i.e. 2304 is the min buffer size of the system
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvbuf, sizeof(recvbuf));
    getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvbuf, (socklen_t*)&len);
    printf("The TCP receive buffer size after setting is: %d\n", recvbuf);

    // bind the sockfd with address(ip and port)
    int ret = bind(sockfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    // set the socket to listen status
    ret = listen(sockfd, 5);
    assert(ret != -1);

    // accept to a client request
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int connfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
    if ( connfd < 0 ) {
        printf("errno is : %d\n", errno);
    } else {
        char buff[BUFF_SIZE];
        memset(buff, '\0', BUFF_SIZE);
        while ( recv(connfd, buff, BUFF_SIZE - 1, 0) > 0 ) {

        }
        close(connfd);
    }
    close(sockfd);
    return 0;
}
