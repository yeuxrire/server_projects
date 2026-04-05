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

void send_response(int client_fd, int status_code, const char *status_text, const char *content_type, const char *body, size_t body_len);

void send_file_response(int client_fd, const char *path);

const char* get_mime_type(const char *path);

#endif