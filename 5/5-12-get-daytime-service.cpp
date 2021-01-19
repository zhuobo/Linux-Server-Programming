#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

int main(int argc, char* argv[]) {
    assert(argc == 2);
    char* host = argv[1];

    // get target host address information
    struct hostent* hostinfo = gethostbyname(host);
    assert(hostinfo);

    // get server information
    struct servent* servinfo = getservbyname("daytime", "tcp");
    assert(servinfo);
    printf("daytime port is %d\n", ntohs(servinfo->s_port));

    // set ip and port to daytime service's ip and port
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = servinfo->s_port;
    address.sin_addr = *(struct in_addr*)*hostinfo->h_addr_list;

    // create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // using the sockfd to connect with daytime address
    int result = connect(sockfd, (struct sockaddr*)&address, sizeof(address));
    assert(result != -1);

    char buffer[128];
    // daytime service will response a string and we read from the sockfd to get it
    result = read(sockfd, buffer, sizeof(buffer));
    assert(result > 0);

    buffer[result] = '\0';
    printf("the daytime is : %s\n", buffer);
    close(sockfd);
    return 0;
}
