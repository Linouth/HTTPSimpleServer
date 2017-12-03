#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/stat.h>

#include "../server.h"
#include "../utils.h"
#include "utils.h"
#include "response.h"
#include "lut.h"


void generate_error_page(struct response *res)
{
    int status_t = res->status_code / 100 - 1;
    int status_c = res->status_code % 100;

    snprintf(res->data, MAX_DATA_SIZE,
             "<html><head>"
             "<title>%d %s</title>"
             "</head><body><center>"
             "<h1>%d %s</h1>"
             "<hr />"
             "%s"
             "</center></body></html>\r\n",
             res->status_code, status[status_t][status_c],
             res->status_code, status[status_t][status_c],
             SERVER_STRING);

    res->data_length += strlen(res->data);
}

void send_status_line(int sfd, struct request *req, struct response *res)
{
    char buf[MAX_STATUS_LINE_SIZE];
    int status_t = (res->status_code / 100) - 1;
    int status_c = res->status_code % 100;

    snprintf(buf, MAX_STATUS_LINE_SIZE,
             "%s %d %s\r\n",
             req->version, res->status_code, 
             status[status_t][status_c]);

    send (sfd, buf, strlen(buf), 0);
}

/* 
 * Send headers in response structure, also add content-length header
 */
void send_header(int sfd, struct response *res)
{
    char buf[MAX_HEADER_KV_SIZE];

    // TODO: Hacky... Fix this shit. No header modifications in a send_header function...
    if (res->data_length > 0) {
        snprintf(buf, MAX_HEADER_KV_SIZE, "%d", res->data_length);
        add_header_entry((struct header_container *)res, "Content-Length", buf);
    }

    // Loop over headers, and send them to client
    int i;
    for (i = 0; i < res->header_count; i++) {
        snprintf(buf, MAX_HEADER_KV_SIZE,
                 "%s: %s\r\n",
                 res->header[i]->key, res->header[i]->value);

        send(sfd, buf, strlen(buf), 0);
    }

    send(sfd, "\r\n", 2, 0);
}

void handle_response(int sfd, struct request *req, struct response *res)
{
    send_status_line(sfd, req, res);

    if (res->status_code >= 400) {
        // Error
        add_header_entry((struct header_container *)res, "Content-Type", "text/html");
        /* add_header_entry((struct header_container *)res, "Connection", "Closed"); */
        generate_error_page(res);

        send_header(sfd, res);
        // TODO: send_data(sfd, res);

        send(sfd, res->data, res->data_length, 0);
    } else if (res->status_code >= 200) {
        // OK respond
        send_header(sfd, res);

        send(sfd, res->data, strlen(res->data), 0);

        if (res->datafile != NULL) {
            // File found and GET request

            char buf[MAX_BUF_SIZE];
            while (fgets(buf, MAX_BUF_SIZE, res->datafile) && buf != NULL) {
                send(sfd, buf, strlen(buf), 0);
            }
        }
    }
}

void prepare_response(struct response *res)
{
    add_header_entry((struct header_container *)res, "Server", SERVER_STRING);
    // TODO: Add date
}

void handle_file(char *filename, struct response *res)
{
    FILE *file;
    struct stat s;

    if (stat(filename, &s) == 0) {
        if (s.st_mode & S_IFDIR) {
            res->status_code = 200;
            strncpy(res->data, "<h1>Directory Listing</h1>", MAX_DATA_SIZE);
            res->data_length += strlen("<h1>Directory Listing</h1>");
        }
        else if (s.st_mode & S_IFREG) {
            // File found, return file.
            file = fopen(filename, "r");
            res->status_code = 200;
            res->datafile = file;

            // Get file size
            int sz;
            fseek(file, 0L, SEEK_END);
            sz = ftell(file);
            res->data_length += sz;
            fseek(file, 0L, SEEK_SET);
        } else {
            printf("Some other item\n");
        }
    } else {
        res->status_code = 404;
    }
}

/*
 * doMETHOD functions to handle specific method requests
 */

void doGET(struct request *req, struct response *res)
{
    if (strlen(req->path) < 2)
        handle_file("index.html", res);
    else
        handle_file(req->path, res);
}

void doHEAD(struct request *req, struct response *res)
{
    if (strlen(req->path) < 2)
        handle_file("index.html", res);
    else
        handle_file(req->path, res);
    res->datafile = NULL;
}


struct response *create_response(struct request *req)
{
    struct response *res = salloc(sizeof(struct response));
    res->header_count = 0;
    res->datafile = NULL;
    res->data[0] = '\0';
    res->data_length = 0;

    prepare_response(res);

    switch(req->method) {
        case GET:
            doGET(req, res);
            break;
        case HEAD:
            doHEAD(req, res);
            break;
    }

    return res;
}
