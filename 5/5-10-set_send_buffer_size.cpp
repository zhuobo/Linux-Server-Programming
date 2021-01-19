// about socket
#include <sys/socket.h>
// about internet programming
#include <arpa/inet.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


#define BUFF_SIZE 512

int main(int argc, char* argv[]) {
    if ( argc <= 2 ) {
        printf("usage: %s ip_address port_number send_buffer_size\n",
                basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);
    
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    // create a socket, sock is a file description
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int sendbuf = atoi(argv[3]);
    int len = sizeof(sendbuf);

    // I set the send buffer size: 2000 and it is 4608 finally
    // that is to say: the final TCP buffer size if double times of min size 2304
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuf, len);
    getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendbuf, (socklen_t*)&len);
    printf(" the tcp send bufer size after setting is %d\n", sendbuf);

    if ( connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) != -1) {
        char buffer[BUFF_SIZE];
        memset(buffer, 'a', BUFF_SIZE);
        send(sock, buffer, BUFF_SIZE, 0);
    }

    close(sock);
    return 0;
}

