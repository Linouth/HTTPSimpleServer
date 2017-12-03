#ifndef SERVER_H_
#define SERVER_H_

#define SERVER_STRING "HTTPSimpleServer/0.0.1"

#define MAX_HEADER_SIZE 2048
#define MAX_PATH_SIZE 1024
#define MAX_VERSION_SIZE 10
#define MAX_STATUS_LINE_SIZE MAX_PATH_SIZE + MAX_VERSION_SIZE + 16
#define MAX_BUF_SIZE MAX_PATH_SIZE + 128
#define MAX_MSG_SIZE 1024
#define MAX_DATA_SIZE 2096

#define MAX_HEADER_KEY_SIZE 256
#define MAX_HEADER_VALUE_SIZE 1024
#define MAX_HEADER_KV_SIZE MAX_HEADER_KEY_SIZE + MAX_HEADER_VALUE_SIZE + 2
#define MAX_HEADER_ENTRIES 32


typedef enum {UNKNOWN, CONNECT, GET, POST, OPTIONS, HEAD, PUT, DELETE, TRACE} method_t;

struct header_entry {
    char *key;
    char *value;
};

struct request {
    struct header_entry *header[MAX_HEADER_ENTRIES];
    int header_count;
    method_t method;
    char version[MAX_VERSION_SIZE];
    char path[MAX_PATH_SIZE];
};

struct response {
    struct header_entry *header[MAX_HEADER_ENTRIES];
    int header_count;
    int status_code;
    FILE *datafile;
    char data[MAX_DATA_SIZE];
};

struct header_container {
    struct header_entry *header[MAX_HEADER_ENTRIES];
    int header_count;
};

#endif
