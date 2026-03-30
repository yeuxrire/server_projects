#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "http_parser.h"

#define BUF_SIZE 8192

void send_response(int client_fd, int status_code, const char *status_text, const char *body) {
    char response[8192];

    snprintf(response, sizeof(response),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: text/html; charset=UTF-8\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             status_code, status_text, strlen(body), body);
    
    write(client_fd, response, strlen(response));
}

void handle_http_request(int client_fd) {
    char buffer[BUF_SIZE];
    int read_len= read(client_fd, buffer, BUF_SIZE - 1);

    if (read_len > 0) {
        buffer[read_len] = '\0';
        char *header_end = strstr(buffer, "\r\n\r\n");

        if(header_end != NULL) {
            *header_end = '\0';

            HttpRequest req;
            memset(&req, 0, sizeof(HttpRequest));

            char *saveptr;

            char *line = strtok_r(buffer, "\r\n", &saveptr);
            if (line == NULL) return;

            int parsed_items = sscanf(line, "%9s %255s %15s", req.method, req.path, req.version);

            if(parsed_items != 3) {
                printf("[ERROR] Malformed Request Line.\n");
                send_response(client_fd, 400, "Bad Request", "<h1>400 Bad Reqeust</h1>");
                return;
            }

            while ((line = strtok_r(NULL, "\r\n", &saveptr)) != NULL) {
                char *colon = strchr(line, ':');
                if(colon != NULL) {
                    *colon = '\0';

                    char *key = line;
                    char *value = colon + 1;

                    while (*value == ' ' || *value == '\t') value++;

                    if (strncasecmp(key, "Content-Length", 14) == 0) {
                        req.content_length = atoi(value);
                    } 
                    else if (strncasecmp(key, "Host", 4) == 0) {
                        strncpy(req.host, value, sizeof(req.host) - 1);
                    }
                    else if (strncasecmp(key, "Connection", 10) == 0) {
                        strncpy(req.connection, value, sizeof(req.connection) - 1);
                    }
                }
            }

            printf("[SUCCESS] Request Line fully parsed!\n");
        
            char success_html[1024];
            snprintf(success_html, sizeof(success_html),
                     "<html><body>"
                     "<h1> Web parsing success!</h1>"
                     "<p> path: <b> %s</b></p>"
                     "</body></html>", req.path);
                     
            send_response(client_fd, 200, "OK", success_html);

        } else {
            printf("[WARNING] Partial header received.\n");
        }
    }
}