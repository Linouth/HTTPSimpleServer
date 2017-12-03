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

#include "server.h"
#include "utils.h"
#include "http/request.h"
#include "http/response.h"
#include "http/lut.h"

char *method_str(method_t m)
{
    return method[m];
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

int main(int argc, char **argv)
{
    struct  sockaddr_storage their_addr;
    socklen_t addr_size;
    int sfd, new_fd;

    char source[INET6_ADDRSTRLEN];

    int bytes_sent;

    init_luts();

    sfd = listen_on_port("1432");

    addr_size = sizeof(their_addr);

    struct request *req;
    struct response *res;
    char buf[MAX_BUF_SIZE];
    for (;;) {
        new_fd = accept(sfd, (struct sockaddr *)&their_addr, &addr_size);
        if (new_fd == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr), source, sizeof(source));

        req = parse_request(new_fd);
        res = create_response(req);
        handle_response(new_fd, req, res);

        int status_t = (res->status_code / 100) - 1;
        int status_c = res->status_code % 100;
        printf("%s\t%s %s %s\t%d %s\n", source, method_str(req->method), req->path, req->version, res->status_code, status[status_t][status_c]);

        free_request(req);
        free_response(res);
        close(new_fd);
    }
}
