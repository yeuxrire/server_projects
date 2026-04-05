#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "http_parser.h"

#define BUF_SIZE 8192

const char* get_mime_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (strcasecmp(ext, ".html") == 0 || strcasecmp(ext, ".htm") == 0) return "text/html";
    if (strcasecmp(ext, ".jpg" == 0) || strcasecmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcasecmp(ext, ".png") == 0) return "image/png";
    if (strcasecmp(ext, ".css") == 0) return "text/css";
    if (strcasecmp(ext, ".js") == 0) return "application/javascript";
    return "text/plain";
}

void send_response(int client_fd, int status_code, const char *status_text, const char *content_type, const char *body, size_t body_len) {
    char header[BUF_SIZE];

    snprintf(header, sizeof(header),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n",
             status_code, status_text, content_type, body_len);
    
    write(client_fd, header, strlen(header));
    if (body && body_len > 0) {
        write(client_fd, body, body_len);
    }
}

void send_file_response(int client_fd, const char *path) {
    char full_path[512] = ".";

    if (strcmp(path, "/") == 0) {
        strcat(full_path, "/index.html");
    } else {
        strcat(full_path, path);
    }

    int file_fd = open(full_path, O_RDONLY);
    if (file_fd == -1) {
        printf("[ERROR] 404 Not Found: %s\n", full_path);
        const char *error_msg = "<h1404 Not Found </h1><p>The requested file was not found on this server.</p>";
        send_response(client_fd, 404, "Not Found", "text/html", error_msg, strlen(error_msg));
        return;
    }

    struct stat file_stat;
    if (fstat(file_fd, &file_stat) == -1) {
        perror("fstat failed");
        close(file_fd);
        return;
    }

    const char *mime_type = get_mime_type(full_path);
    char header[BUF_SIZE];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %11d\r\n"
             "Connection: close\r\n"
             "\r\n",
             mime_type, (long long)file_stat.st_size);
    write(client_fd, header, strlen(header));

    char file_buf[BUF_SIZE];
    ssize_t read_bytes;
    while ((read_bytes = read(file_fd, file_buf, size_of(file_buf))) > 0) {
        write(client_fd, file_buf, read_bytes);
    }

    close(file_fd);

    printf("[SUCCESS] File sent: %s (%lld bytes)\n", full_path, (long long)file_stat.st_size);
}

void handle_http_request(int client_fd) {
    char buffer[BUF_SIZE];
    int read_len = read(client_fd, buffer, BUF_SIZE - 1);

    if (read_len > 0) {
        buffer[read_len] = '\0';
        char *header_end = strstr(buffer, "\r\n\r\n");

        if (header_end == NULL) return;

        *header_end = '\0';
        HttpRequest req;
        memset(&req, 0, sizeof(HttpRequest));

        char *saveptr;
        char *line = strtok_r(buffer, "\r\n", &saveptr);
        
        if (line == NULL) return;

        if (sscanf(line, "%9s %255s %15s", req.method, req.path, req.version) == 3) {
            send_file_response(client_fd, req.path);
        }
    }
}