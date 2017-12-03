#ifndef UTILS_H_
#define UTILS_H_

void *salloc(size_t size);
void sfree(void *ptr);
void free_request(struct request *req);
void free_response(struct response *req);

#endif
