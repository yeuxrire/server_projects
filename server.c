#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "http_parser.h"

int server_fd;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void sigint_handler(int sig) {
    printf("\n[Server] Closing server socket (FD: %d) and exiting ...\n", server_fd);
    close(server_fd);
    pthread_mutex_destroy(&log_mutex);
    exit(0);
}

void *client_handler(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    pthread_detach(pthread_self());

    pthread_mutex_lock(&log_mutex);
    printf("[Thread %lu] Connected to client.\n", pthread_self());
    pthread_mutex_unlock(&log_mutex);

    handle_http_request(client_fd);

    pthread_mutex_lock(&log_mutex);
    printf("[Thread %lu] Client is disconnected.\n", pthread_self());
    pthread_mutex_unlock(&log_mutex);

    close(client_fd);
    pthread_exit(NULL);
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    struct sigaction sa_int;
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(1);
    }

    listen(server_fd, 5);
    printf("Server Started. Waiting for client (Multi-Threaded)...\n");

    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd == -1) continue;

        int *new_sock = malloc(sizeof(int));
        *new_sock = client_fd;

        pthread_t thread_id;

        if (pthread_create(&thread_id, NULL, client_handler, (void *)new_sock) != 0) {
            perror("Thread creation failed");
            free(new_sock);
            close(client_fd);
        }
    }

    close(server_fd);

    return 0;
}