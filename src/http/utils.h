#ifndef HTTP_UTILS_H_
#define HTTP_UTILS_H_

int add_header_entry(struct header_container *c, char *k, char *v);
int recv_line(int sfd, char *out, int size);

#endif
