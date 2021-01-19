#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h> // for writev

#define BUFF_SIZE 1024
////////////////////////////////////////////////////////////////////////////////
// writev(int fd, const struct iovec* vector, int count);
// readv(int fd, const structr iovec* vector, int count);
//
// write/read content from different address.
// eg. HTTP response message: 1 status line, many headers, 1 empty line, content
// These 4 part may in different address, we don't have to cat them together.
// writev() can write the together. 
//
// !!! we have to alloc a buffer in userspace to store all the content we want 
// to write to socket

static const char* status_line[2] = { "200 OK", "500 Internal server error" };

int main(int argc, char* argv[]) {
    if ( argc <= 3 ) {
        printf("usage: ./%s ip_address port filename", basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);
    const char* file_name = argv[3];

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd >= 0);

    int ret = bind(sockfd, (struct sockaddr*)&address, sizeof(address));
    assert( ret != -1 );

    ret = listen(sockfd, 5);
    assert( ret != -1);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int connfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
    if ( connfd < 0 ) {
        printf("connection failed! errno is %d\n", errno);
    } else {
        // use to save HTTP response status lines, headers and empty line
        char header_buff[BUFF_SIZE];
        memset(header_buff, '\0', BUFF_SIZE);

        // user to save HTTP response file content
        char* file_buff = nullptr;
        // 
        struct stat file_stat;

        // target file is valid
        bool valid = true;

        // header buffer used size
        int len = 0;

        // target file does not exist
        if ( stat(file_name, &file_stat) < 0 ) {
            valid = false;
        } else {
            if ( S_ISDIR(file_stat.st_mode) ) {
                valid = false;
            } else if (file_stat.st_mode & S_IROTH ) {
               // cur user has permision to read the file
                int fd = open(file_name, O_RDONLY);
                file_buff = new char[file_stat.st_size + 1];
                memset(file_buff, '\0', file_stat.st_size + 1);
                if ( read(fd, file_buff, file_stat.st_size) < 0 ) {
                    valid = false;
                }

            } else {
                valid = false;
            }
        }

        // if target file exist, send HTTP response message
        if ( valid ) {
            ret = snprintf(header_buff, BUFF_SIZE - 1, "%s %s\r\n",
                    "HTTP/1.1", status_line[0]);
            len += ret;
            ret = snprintf(header_buff + len, BUFF_SIZE - 1 - len, "%s", "\r\n");

            // writev write header_buffer and file_buffer together
            struct iovec iv[2];
            iv[0].iov_base = header_buff;
            iv[0].iov_len = strlen(header_buff);
            iv[1].iov_base = file_buff;
            iv[1].iov_len = file_stat.st_size;
            ret = writev(connfd, iv, 2);
        } else {
            ret = snprintf(header_buff, BUFF_SIZE - 1, "%s %s\r\n",
                    "HTTP/1.1", status_line[1]);
            len += ret;
            ret = snprintf(header_buff + len, BUFF_SIZE - 1 - len, "%s", "\r\n");
            send(connfd, header_buff, strlen(header_buff), 0);
        }
        close(connfd);
        delete [] file_buff;
    }
    close(sockfd);
    return 0;
}
