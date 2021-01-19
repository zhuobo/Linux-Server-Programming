#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>



int main() {
    const char* ip = "127.0.0.1";
    int port = 12345;
    const char* file_name = "b.txt";

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    if ( connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != -1 ) {
       send(sockfd, file_name, strlen(file_name), 0);
       char buffer[1024];
       memset(buffer, '\0', 1024);
       int n = recv(sockfd, buffer, 1024, 0);
       printf("%s", buffer);
    }
    close(sockfd);
}
