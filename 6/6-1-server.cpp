#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


////////////////////////////////////////////////////////////////////////////////
// This server grogram is to show the usage of I/O functin dup(). dup(int fd)
// will create a new file description and the new fd will also point the file
// argument fd points to. Especially, dup() will alway return the smallest fd
// of current system. As the program show, we firstly close std_out, which fd
// is 1. Now 1 is the smallest fd in the system, and we call the function dup
// after that. New fd is 1 now! printf() alway write to fd == 1, so all cont-
// ent will redirect to client.
// ////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
    if ( argc <= 2 ) {
        printf("usage: ./%s ip_address port_number", basename(argv[0]));
        return 0;
    }
    
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    // set address famly, port, ip
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);
    
    // bind the socket with a given addr
    int ret = bind(sockfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    // listen to the socket
    ret = listen(sockfd, 5);
    assert(ret != -1);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int connfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
    if ( connfd < 0 ) {
        printf("connect failed! The errno is : %d", errno);
    } else {
        // close std_out file description, now 1 is the smallest fd
        close(STDOUT_FILENO);
        // duplicate a new fd points to connfd
        int new_fd = dup(connfd);
        // string "abcd" will to fd == 1,will redirect to connfd
        printf("abcd\n");
        close(connfd);

    }

    close(sockfd);
    return 0;
}
