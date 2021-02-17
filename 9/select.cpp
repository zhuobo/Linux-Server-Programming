#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if ( argc <= 2 ) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    const int port = atoi(argv[2]);

    int ret = 0;

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
    if  ( connfd < 0 ) {
        printf("connect fail, errno is %d", errno);
        close(listenfd);
    }

    char buff[1024];
    fd_set read_fds;
    fd_set exception_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&exception_fds);

    while ( 1 ) {
        memset(buff, '\0', sizeof(buff));
        FD_SET(connfd, &read_fds);
        FD_SET(connfd, &exception_fds);
        ret = select(connfd + 1, &read_fds, NULL, &exception_fds, NULL);
        if ( ret < 0 ) {
            printf("connection failure, errno is %d", errno);
            break;
        }

        if ( FD_ISSET(connfd, &read_fds) ) {
            ret = recv(connfd, buff, sizeof(buff) - 1, 0);
            if ( ret <= 0 ) {
                break;
            }
            printf("get %d bytes of normal data : %s\n", ret, buff);
        } else if ( FD_ISSET(connfd, &exception_fds) ) {
            ret = recv(connfd, buff, sizeof(buff) - 1, 0);
            if ( ret <= 0 ) {
                break;
            }
            printf("get %d bytes oob data: %s/n", ret, buff);
        }
    }
    close(connfd);
    close(listenfd);
    return 0;


}
