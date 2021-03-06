#include "http_conn.h"


// implement of class http_conn

const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or in inherently impossible to satisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file from this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this serve.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There wa an unusual problem serving the requested file.\n";
const char *doc_root = "/val/www/html";

int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

// add fd to epollfd(kernel event table)
// set one shot in fd
void addfd(int epollfd, int fd, bool one_shot) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if ( one_shot ) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

// remove fd in epollfd
void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

// modify fd in epollfd
void modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

void http_conn::close_conn(bool real_close) {
    if ( real_close && ( m_sockfd != -1 ) ) {
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        --m_user_count;
    }
}

void http_conn::init(int sockfd, const sockaddr_in& addr) {
    m_sockfd = sockfd;
    m_address = addr;
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    addfd(m_epollfd, sockfd, true);
    ++m_user_count;

    init();
}

void http_conn::init() {
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;

    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
}

http_conn::LINE_STATUS http_conn::parse_line() {
    char temp;
    for ( ;m_checked_idx < m_read_idx; ++m_checked_idx ) {
        temp = m_read_buf[m_checked_idx];
        if ( temp =='\r' ) {
            if ( (m_checked_idx + 1) == m_read_idx ) {
                return LINE_OPEN;
            } else if ( m_read_buf[m_checked_idx + 1] == '\n' ) {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK; 
            }
            return LINE_BAD;
        } else if ( temp == '\n' ) {
            if ( (m_checked_idx > 1) && (m_read_buf[m_checked_idx - 1] == '\r') ) {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++]= '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

// read data client socket
bool http_conn::read() {
    if ( m_read_idx >= READ_BUFFER_SIZE ) {
        return false;
    }
    int bytes_read = 0;
    while ( true ) {
        bytes_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        if ( bytes_read == -1 ) {
            if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
                break;
            } else if ( bytes_read == 0 ) {
                return false;
            }
            m_read_idx += bytes_read;
        }
    }
    return true;
}


// parse http request line,get request method, url, http version
http_conn::HTTP_CODE http_conn::parse_request_line(char *text) {
    m_url = strpbrk(text, " \t");
    if ( !m_url ) {
        return BAD_REQUEST;
    }
    *m_url++ = '\0';

    char* method = text;
    if ( strcasecmp(method, "GET") == 0 ) {
        m_method = GET;
    } else {
        return BAD_REQUEST;
    }

    m_url += strspn(m_url, " \t");
    m_version = strpbrk(m_url, " \t");
    if ( !m_version ) {
        return BAD_REQUEST;
    }
    *m_version++ = '\0';
    m_version += strspn(m_version, " \t");
    if ( strcasecmp(m_version, "HTTP/1.1") != 0 ) {
        return BAD_REQUEST;
    }
    if ( strncasecmp(m_url, "http://", 7) == 0 ) {
        m_url += 7;
        m_url = strchr(m_url, '/');
    }

    if ( !m_url || m_url[0] != '/' ) {
        return BAD_REQUEST;
    }
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

// parse http header
http_conn::HTTP_CODE http_conn::parse_headers(char *text) {
    // empty line 
    if ( text[0] == '\0' ) {
        // we have to read content of the request
        if ( m_content_length != 0 ) {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    } else if ( strncasecmp(text, "Connection:", 11) == 0 ) { // header
        text += 11;
        text += strspn(text, " \t");
        if ( strcasecmp(text, "keep-alive") == 0 ) {
            m_linger = true;
        }
    } else if ( strncasecmp(text, "Content-Length:", 15) == 0 ) {
        text += 15;
        text += strspn(text, " \t");
        m_content_length += atol(text);
    } else if ( strncasecmp(text, "Host:", 5) == 0 ) {
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    } else {
        printf("oop! unknow header %s\n", text);
    }
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::parse_content(char* text) {
    if ( m_read_idx >= (m_content_length + m_checked_idx) ) {
        text[m_content_length] = '\0';
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

// state machine
http_conn::HTTP_CODE http_conn::process_read() {
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = 0;

    while ( ( m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK ) 
            || (line_status = parse_line()) == LINE_OK ) {
        text = get_line();
        m_start_line = m_checked_idx;
        printf("got 1 http line: %s\n", text);
        switch(m_check_state) {
        case CHECK_STATE_REQUESTLINE: 
            ret = parse_request_line(text);
            if ( ret == BAD_REQUEST ) {
                return BAD_REQUEST;
            }
            break;
        case CHECK_STATE_HEADER:
            ret = parse_headers(text);
            if ( ret == BAD_REQUEST ) {
                return BAD_REQUEST;
            } else if ( ret == GET_REQUEST ) {
                return do_request();
            }
            break;
        case CHECK_STATE_CONTENT:
            ret = parse_content(text);
            if ( ret == GET_REQUEST ) {
                return do_request();
            }
            line_status = LINE_OPEN;
            break;
        default:
            return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}


// do_request
http_conn::HTTP_CODE http_conn::do_request() {
    strcpy(m_real_file, doc_root);
    int len = strlen(doc_root);
    strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);
    if ( stat(m_real_file, &m_file_stat) < 0 ) {
        return NO_REQUEST;
    }

    if ( !(m_file_stat.st_mode & S_IROTH) ) {
        return FORBIDDEN_REQUEST;
    }

    if ( S_ISDIR(m_file_stat.st_mode) ) {
        return BAD_REQUEST;
    }

    int fd = open(m_real_file, O_RDONLY);
    m_
}
