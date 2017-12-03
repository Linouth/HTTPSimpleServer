#include <stdio.h>
#include <stdlib.h>

#include "server.h"

void *salloc(size_t size)
{
    void *buf = malloc(size);
    if (buf == NULL) {
        fprintf(stderr, "ERROR: Failed to allocate memory.\n");
        exit(EXIT_FAILURE);
    }
    return buf;
}


void sfree(void *ptr) {
    if (ptr)
        free(ptr);
    else
        fprintf(stderr, "ERROR: sfree target is NULL\n");
}

void free_request(struct request *req)
{
    int i;
    for (i = 0; i < req->header_count; i++) {
        sfree(req->header[i]->key);
        sfree(req->header[i]->value);
        sfree(req->header[i]);
    }
    sfree(req);
}

void free_response(struct response *res)
{
    int i;
    for (i = 0; i < res->header_count; i++) {
        sfree(res->header[i]->key);
        sfree(res->header[i]->value);
        sfree(res->header[i]);
    }
    sfree(res->datafile);
    sfree(res);
}
