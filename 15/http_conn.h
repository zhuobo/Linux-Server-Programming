#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "locker.h"

class http_conn {
public:
    // longest file name
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    // http request method
    enum METHOD {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATCH
    };
    // host status
    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    // possible result 
    enum HTTP_CODE {
        NO_REQUEST, GET_REQUEST, BAD_REQUEST,
        NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST,
        INTERNAL_ERROR, CLOSED_CONNECTION
    };
    // line status
    enum LINE_STATUS {
        LINE_OK = 0,
        LINE_BAD, 
        LINE_OPEN
    };

public:
    http_conn() {  }
    ~http_conn() {  }

public:
    // init new connection
    void init(int sockfd, const sockaddr_in& addr);
    void close_conn(bool real_close = true);
    // handle client requese
    void process();
    // nonblocking read
    bool read();
    // nonblocking write
    bool write();

private:
    void init();
    // process http request
    HTTP_CODE process_read();
    // construct http respond
    bool process_write(HTTP_CODE ret);

    // called by prcess_read();
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char* get_line() {
        return m_read_buf + m_start_line;
    }
    LINE_STATUS parse_line();

    // called by process_write
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_len);
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

public:
    // all the event in all socket are registered to the same epoll kernel table
    static int m_epollfd;

    // client count
    static int m_user_count;

private:
    // socket of server and client
    int m_sockfd;
    sockaddr_in m_address;

    // read buff
    char m_read_buf[READ_BUFFER_SIZE];
    // the final index of client data
    int m_read_idx;
    // the position of the char being parse in readbuf
    int m_checked_idx;
    // the start position of the line being parse
    int m_start_line;
    // write buffer
    char m_write_buf[WRITE_BUFFER_SIZE];
    // bytes to send in write buf
    int m_write_idx;

    // cur state of sever
    CHECK_STATE m_check_state;
    // request method
    METHOD m_method;

    // the complete path of client request
    char m_real_file[FILENAME_LEN];
    // request file name
    char *m_url;
    // http version
    char *m_version;
    // hostname
    char *m_host;
    // message length
    int m_content_length;
    // keek live
    bool m_linger;

    // the mmap position of request file
    char *m_file_address;
    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;
};




#endif
