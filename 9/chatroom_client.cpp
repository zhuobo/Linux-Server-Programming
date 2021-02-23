#define _GNU_SOURCE 1

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <fcntl.h>

#define BUFF_SIZE 1024

int main(int argc, char* argv[]) {
    if ( argc <= 2 ) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    const int port = atoi(argv[2]);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);
    if ( connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ) {
        printf("connection failed\n");
        close(sockfd);
        return 1;
    }

    // poll 2 fds, std_in and server
    pollfd fds[2];
    fds[0].fd = 0;             // fd number
    fds[0].events = POLLIN;    // events on this fd
    fds[0].revents = 0;        // events actually happened
    fds[1].fd = sockfd;
    fds[1].events = POLLIN | POLLRDHUP;
    fds[1].revents = 0;

    char read_buff[BUFF_SIZE];
    int pipefd[2];
    int ret = pipe(pipefd);
    assert(ret != -1);

    while ( 1 ) {
        ret = poll(fds, 2, -1); // block poll until events happen
        if ( ret < 0 ) {
            printf("poll failure\n");
            break;
        }

        if ( fds[1].revents & POLLRDHUP ) {
            printf("server close the connection\n");
            break;
        } else if ( fds[1].revents & POLLIN ) {
            memset(read_buff, '\0', BUFF_SIZE);
            recv(fds[1].fd, read_buff, BUFF_SIZE - 1, 0);
            printf("%s\n", read_buff);
        }

        if ( fds[0].revents & POLLIN ) {
            // from std_in to fipe
            ret = splice(0, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
            // from pipe to sockfd
            ret = splice(pipefd[0], NULL, sockfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        }
        
    }
    close(sockfd);
    return 0;
}
