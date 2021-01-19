#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

#define MAXLINE 1024

int main(int argc, char* argv[]) {
    if ( argc <= 3 ) {
        printf("usage:./%s ip_address port_number message\n",
                basename(argv[0]));
        return 1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    char *message = argv[3];

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);
    char buff[MAXLINE];
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    int connfd = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)); 
    write(sockfd, message, strlen(message));
    int n = read(sockfd, buff, MAXLINE - 1);
    write(STDOUT_FILENO, buff, n);
    printf("\n");
    close(sockfd);
    return 0;
}
