#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#define MAX_HEADER_SIZE 2048
#define MAX_PATH_SIZE 1024
#define MAX_BUF_SIZE MAX_PATH_SIZE + 128
#define MAX_MSG_SIZE 1024
#define MAX_DATA_SIZE 2096

#define SERVER_STRING "HTTPSimpleServer/0.0.1"

typedef enum {UNKNOWN, CONNECT, GET, POST, OPTIONS, HEAD, PUT, DELETE, TRACE} method_t;

struct request {
    method_t method;
    char header[MAX_HEADER_SIZE];
    int header_size;
    char path[MAX_PATH_SIZE];
};

struct response {
    int status_code;
    char header[MAX_HEADER_SIZE];
    int header_size;
    FILE *datafile;
    char data[MAX_DATA_SIZE];
};

char *errors[50] = {0};

void init_response_codes() {
    errors[0] = "Bad Request";
    errors[1] = "Unauthorized";
    errors[2] = "Payment Required";
    errors[3] = "Forbidden";
    errors[4] = "Not Found";
}

void *get_in_addr(struct sockaddr *sa)
{
    if(sa->sa_family == AF_INET)
        return &(((struct sockaddr_in *)sa)->sin_addr);

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int listen_on_port(char *port)
{
    struct addrinfo hints, *servaddr, *sp;
    int error, sfd;
    int yes=1;

    // Prepare hints struct
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // Get addrinfo for listening to port 'port'
    error = getaddrinfo(NULL, port, &hints, &servaddr);
    if (error != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }

    // Try addrinfo's in linked list till one binds
    for (sp = servaddr; sp != NULL; sp = sp->ai_next) {
        sfd = socket(sp->ai_family, sp->ai_socktype, sp->ai_protocol);
        if (sfd == -1)
            continue;

        // Enable socket reuse, prevents annoying errors
        if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            return -1;
        }

        if (bind(sfd, sp->ai_addr, sp->ai_addrlen) == 0)
            break;

        close(sfd);
    }

    if (sp == NULL) {
        fprintf(stderr, "Could not bind\n");
        return -1;
    }

    freeaddrinfo(servaddr);

    if (listen(sfd, 5) == -1) {
        perror("listen");
        return -1;
    }

    return sfd;
}

int recv_line(int sfd, char *out, int size)
{
    char c;
    int i = 0;

    while (recv(sfd, &c, 1, 0) > 0 && i < size-1) {
        out[i] = c;
        i++;
        if (c == '\n')
            break;
    }

    out[i] = '\0';
    return i;
}

struct response *doGET(struct request *req)
{
    struct response *res = malloc(sizeof(struct response));
    res->datafile = NULL;

    // TODO: Temp
    strncpy(req->path, "server.c", MAX_PATH_SIZE);

    FILE *file;
    if ((file = fopen(req->path, "r")) == NULL) {
        if (errno == ENOENT) {
            // 404: File not found.
            res->status_code = 404;
        } else if (errno == EACCES) {
            // 403: Forbidden.
            res->status_code = 403;
        } else if (errno == EISDIR) {
            // List Directory
            res->status_code = 200;
        }
    } else {
        // File found, return file.
        res->status_code = 200;
        res->datafile = file;
    }


    return res;
}

void send_error(int sfd, struct response *res)
{
    char header[MAX_HEADER_SIZE];
    char msg[MAX_MSG_SIZE];
    int code_index = res->status_code % 100;

    snprintf(msg, MAX_MSG_SIZE,
             "<html><head>"
             "<title>%d %s</title>"
             "</head><body><center>"
             "<h1>%d %s</h1>"
             "<hr />"
             "%s"
             "</center></body></html>",
             res->status_code, errors[code_index],
             res->status_code, errors[code_index],
             SERVER_STRING);

    snprintf(header, MAX_HEADER_SIZE,
             "HTTP/1.1 %d %s\r\n"
             "Server: %s\r\n"
             "Content-Length: %d"
             "Content-Type: text/html"
             "Connection: Closed",
             res->status_code, errors[code_index],
             SERVER_STRING, strlen(msg));

    send(sfd, header, strlen(header), 0);
    send(sfd, "\r\n\r\n", 4, 0);
    send(sfd, msg, strlen(header), 0);
}

void send_header(int sfd, struct response *res)
{
    // TODO: Add header to response struct, this function is temporary.
    // TODO: Lookup table for all messages, not just errors...
    char header[MAX_HEADER_SIZE];
    int code_index = res->status_code % 100;

    snprintf(header, MAX_HEADER_SIZE,
             "HTTP/1.1 %d %s\r\n"
             "Server: %s\r\n"
             "Connection: Closed\r\n\r\n",
             res->status_code, errors[code_index],
             SERVER_STRING);

    send(sfd, header, strlen(header), 0);
}

void handle_response(int sfd, struct response *res)
{
    if (res->status_code >= 400) {
        // Error
        send_error(sfd, res);
    } else if (res->status_code >= 200) {
        // OK respond
        if (res->datafile != NULL) {
            // File found
            send_header(sfd, res);

            char buf[MAX_BUF_SIZE];
            while (fgets(buf, MAX_BUF_SIZE, res->datafile) && buf != NULL) {
                send(sfd, buf, strlen(buf), 0);
            }
        }
    }
}

void handle_request(int sfd)
{
    struct request *req = malloc(sizeof(struct request));
    struct response *res;

    char first_line[MAX_BUF_SIZE];
    char buf[MAX_BUF_SIZE];
    char *methodp, *pathp, *versionp, *p;

    /* 
     * Recv first line and split into method, path and version 
     */
    recv_line(sfd, first_line, MAX_BUF_SIZE);

    versionp = strstr(first_line, "HTTP/");
    p = strstr(versionp, "\r\n");
    *p = '\0';

    methodp = strtok(first_line, " ");

    *(versionp - 1) = '\0';
    pathp = methodp + strlen(methodp) + 1;

    /*
     * Fill in the request structure
     */

    // Fill in path
    strcpy(req->path, pathp);

    // Fill in header
    req->header_size = 1;
    int chars_read;
    while ((chars_read = recv_line(sfd, buf, MAX_BUF_SIZE)) > 2) {
        if (req->header_size + chars_read > MAX_HEADER_SIZE)
            // Prevent header from overflowing
            break;

        strcat(req->header, buf);
        req->header_size += chars_read;
    }

    // Fill in method and call proper function
    if (strcmp(methodp, "GET") == 0) {
        req->method = GET;
        res = doGET(req);
    }
    else if (strcmp(methodp, "POST") == 0)
        req->method = POST;
    else if (strcmp(methodp, "CONNECT") == 0)
        req->method = CONNECT;
    else if (strcmp(methodp, "OPTIONS") == 0)
        req->method = OPTIONS;
    else if (strcmp(methodp, "HEAD") == 0)
        req->method = HEAD;
    else if (strcmp(methodp, "PUT") == 0)
        req->method = PUT;
    else if (strcmp(methodp, "DELETE") == 0)
        req->method = DELETE;
    else if (strcmp(methodp, "TRACE") == 0)
        req->method = TRACE;

    int errors_ind = res->status_code % 100;
    printf("127.0.0.1\t%s %s %s\t%d %s\n", methodp, pathp, versionp, res->status_code, errors[errors_ind]);

    handle_response(sfd, res);

    free(req);
    free(res);
}

int main(int argc, char **argv)
{
    struct  sockaddr_storage their_addr;
    socklen_t addr_size;
    int sfd, new_fd;

    char addr_s[INET6_ADDRSTRLEN];

    int bytes_sent;

    init_response_codes();

    sfd = listen_on_port("1432");

    addr_size = sizeof(their_addr);

    for (;;) {
        new_fd = accept(sfd, (struct sockaddr *)&their_addr, &addr_size);
        if (new_fd == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr), addr_s, sizeof(addr_s));

        printf("Connection from %s\n", addr_s);

        handle_request(new_fd);
        close(new_fd);
    }
}
