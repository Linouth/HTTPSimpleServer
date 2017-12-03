#ifndef RESPONSE_H_
#define RESPONSE_H_

char *errors[50];

struct response *create_response(struct request *req);
void handle_response(int sfd, struct request *req, struct response *res);

#endif
