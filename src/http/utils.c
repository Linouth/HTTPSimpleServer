#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "../server.h"
#include "../utils.h"


int add_header_entry(struct header_container *c, char *k, char *v)
{
    struct header_entry *entry;

    if (c->header_count < MAX_HEADER_ENTRIES) {
        c->header[c->header_count] = salloc(sizeof(struct header_entry));
        entry = c->header[c->header_count];
        c->header_count++;
    } else {
        fprintf(stderr, "Error: Too many header lines\n");
        return -1;
    }

    entry->key = strndup(k, MAX_HEADER_KEY_SIZE-1);
    entry->value = strndup(v, MAX_HEADER_KEY_SIZE-1);
    return 0;
}

int recv_line(int sfd, char *out, int size)
{
    char c;
    int i = 0;
    int check = 0;

    while (recv(sfd, &c, 1, 0) > 0 && i < size-1) {
        out[i] = c;
        i++;

        if (check && c == '\n')
            break;

        if (c == '\r')
            check = 1;
        else
            check = 0;
    }

    out[i] = '\0';
    return i;
}
