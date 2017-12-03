#include <stdlib.h>
#include <stdio.h>

#include "../server.h"
#include "lut.h"


char *success[10] = {0};
char *redirect[9] = {0};
char *client_error[50] = {0};
char *server_error[12] = {0};

void init_methods()
{
    method[GET] = "GET";
    method[PUT] = "PUT";
    method[POST] = "POST";
    method[HEAD] = "HEAD";
    method[TRACE] = "TRACE";
    method[DELETE] = "DELETE";
    method[OPTIONS] = "OPTIONS";
    method[CONNECT] = "CONNECT";
    method[UNKNOWN] = "UNKNOWN";
}

void init_response_codes() {
    status[0] = NULL;
    status[1] = success;
    status[2] = redirect;
    status[3] = client_error;
    status[4] = server_error;

    success[0] = "OK";
    success[1] = "Created";
    success[2] = "Accepted";
    success[3] = "Non-Authorative information";
    success[4] = "No Content";
    success[5] = "Reset Content";

    redirect[0] = "Multiple Choices";
    redirect[1] = "Moved Permanently";
    redirect[2] = "Found";
    redirect[3] = "See Other";
    redirect[4] = "Not Modified";

    client_error[0] = "Bad Request";
    client_error[1] = "Unauthorized";
    client_error[2] = "Payment Required";
    client_error[3] = "Forbidden";
    client_error[4] = "Not Found";
    client_error[5] = "Method Not Allowed";
    client_error[18] = "I'm a teapot";

    server_error[0] = "Internal Server Error";
    server_error[1] = "Not Implemented";
}


void init_luts()
{
    init_methods();
    init_response_codes();
}
