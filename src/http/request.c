#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../server.h"
#include "../utils.h"
#include "utils.h"


void parse_header(int sfd, struct request *req)
{
    char buf[MAX_BUF_SIZE];
    char *k, *v;


    int chars_read;
    while ((chars_read = recv_line(sfd, buf, MAX_BUF_SIZE)) > 2) {
        // TODO: Split header into key and value; Save them in dict
        k = strtok(buf, " ");
        k[strlen(k)-1] = '\0';

        v = strtok(NULL, "\r\n");

        // TODO: Error checking
        add_header_entry((struct header_container *)req, k, v);
    }
}

struct request *parse_request(int sfd)
{
    struct request *req = salloc(sizeof(struct request));
    req->header_count = 0;

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

    // Remove leading '/'
    while (*pathp == '/')
        pathp++;
    /*
     * Fill in the request structure
     */

    // Fill in path
    strncpy(req->path, pathp, MAX_PATH_SIZE);

    // Fill in version
    strncpy(req->version, versionp, MAX_VERSION_SIZE);

    // Fill in header
    parse_header(sfd, req);

    // TODO: Find request data
    //

    // Fill in method
    if (strcmp(methodp, "GET") == 0)
        req->method = GET;
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

    return req;
}
