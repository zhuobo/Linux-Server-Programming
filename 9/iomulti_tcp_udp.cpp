///////////////////////////////////////////////////////////////////////////////
// Intruduction:
//     This is a retroreflective server program that can return data sent to the
//     program. This program uses epoll to process TCP requests and UDP requests
//     at the same time.
// Usage:
//     1. compile
//     2. run: ./basename ip_address port_number
//     3. connect: telnet ip_address port_number
///////////////////////////////////////////////////////////////////////////////
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <fcntl.h>


#define MAX_EVENT_NUMBER 1024
#define TCP_BUFFER_SIZE 512
#define UDP_BUFFER_SIZE 1024

// set the fd open as nonblocking
int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, old_option | O_NONBLOCK);
    return old_option;
}

// register fd to epollfd(kernel event table)
void addfd(int epollfd, int fd) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

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

    // create TCP socket and bind it to port
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    // create UDP socket and bind the UDP socket to port
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);
    int udpfd = socket(PF_INET, SOCK_DGRAM, 0);
    assert(udpfd >= 0);

    ret = bind(udpfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);

    addfd(epollfd, listenfd);
    addfd(epollfd, udpfd);

    while ( true ) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if ( number < 0 ) {
            printf("epoll failure\n");
            break;
        }

        for ( int i = 0; i < number; ++i ) {
            int sockfd = events[i].data.fd;
            if ( sockfd == listenfd ) { 
                struct sockaddr_in client_addr;
                socklen_t client_addrlen = sizeof(client_addr);
                int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addrlen);
                addfd(epollfd, connfd);
            } else if ( sockfd == udpfd ) {
                char buff[UDP_BUFFER_SIZE];
                memset(buff, '\0', UDP_BUFFER_SIZE);
                struct sockaddr_in client_addr;
                socklen_t client_addrlen = sizeof(client_addr);
                ret = recvfrom(udpfd, buff, UDP_BUFFER_SIZE - 1, 0, (struct sockaddr*)&client_addr, &client_addrlen);
                if ( ret > 0 ) {
                    sendto(udpfd, buff, UDP_BUFFER_SIZE - 1, 0, (struct sockaddr*)&client_addr, client_addrlen);
                }
            } else if ( events[i].events & EPOLLIN ) { // tcp
                char buff[TCP_BUFFER_SIZE];
                while ( true ) {
                    memset(buff, '\0', TCP_BUFFER_SIZE);
                    ret = recv(sockfd, buff, TCP_BUFFER_SIZE - 1, 0);
                    if ( ret < 0 ) {
                        if ( (errno == EAGAIN) || (errno == EWOULDBLOCK) ) {
                            break;
                        }
                        close(sockfd);
                        break;
                    } else if ( ret == 0 ) {
                        close(sockfd);
                    } else {
                        send(sockfd, buff, ret, 0);
                    }
                }
            } else {
                printf("something else happended\n");
            }
        }
    }
    close(listenfd);
    return 0;
}
