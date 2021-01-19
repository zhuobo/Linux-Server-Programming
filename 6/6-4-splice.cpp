#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>


int main(int argc, char* argv[]) {
    if ( argc < 3 ) {
        printf("usage: %s ip_address port\n", basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port  = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    int ret = bind(sockfd, (struct sockaddr*)&address, sizeof(address));
    assert( ret != -1);

    ret = listen(sockfd, 5);
    assert(ret != -1);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int connfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
    if ( connfd < 0 ) {
        printf("connection failed! errno is %d\n", errno);
    } else {
        while ( true ) {
    // splice
            int pipefd[2];
            // create a pipe between pipefd[0] and pipefd[1]
            ret = pipe(pipefd);
            assert(ret != -1);

            // direct the data of connfd to the pipe
            ret = splice(connfd, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
            assert(ret != -1);

            ret = splice(pipefd[0], NULL, connfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
            assert(ret != -1);

            // close(connfd);

        }

    }

    close(sockfd);
    return 0;
}
