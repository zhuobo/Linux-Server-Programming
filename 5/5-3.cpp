#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <iostream>


static bool stop = false;

static void handle_term( int sig ) {
    stop = true;
}

int main(int argc, char* argv[]) {
    signal( SIGTERM, handle_term );
    if ( argc <= 3 ) {
        printf("usage: %s ip_address port_number backlog\n", basename( argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int backlog = atoi(argv[3]);
    
    // 协议族用来指定socket的协议
    // PF_INET表示ipv4
    // sock_stream: tcp协议，sock_ugram,udp协议
    int sock = socket( PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    // 地址族用来指定某个地址对应的协议
    // 绑定socket到某个具体的地址
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    
    // ip地址写到第三个参数中
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    // 绑定参数分别是 socket socket地址，socket地址长度
    int ret = bind(sock, (struct sockaddr*) &address, sizeof(address));
    assert(ret != -1);

    // 监听socket，创建一个监听队列
    ret = listen(sock, backlog);
    assert(ret != -1);

    while ( !stop ) {
        sleep(1);
    }

    close(sock);
    return 0;
}
