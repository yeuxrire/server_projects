#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

typedef struct {
    char method[10];
    char path[256];
    char version[16];

    char host[256];
    int content_length;
    char connection[64];
} HttpRequest;

void handle_http_request(int client_fd);

#endif