#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int server_fd;

void child_handler(int sig) {
    int status;
    pid_t pid;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("Child Process %d terminated.\n", pid);
        fflush(stdout);
    }
}

void sigint_handler(int sig) {
    printf("\n[Server] Closing server socket (FD: %d) and exiting ...\n", server_fd);
    close(server_fd);
    exit(0);
}

int main() {
    int client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    struct sigaction sa_child;
    sa_child.sa_handler = child_handler;
    sigemptyset(&sa_child.sa_mask);
    sa_child.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa_child, NULL) == -1) {
        perror("sigaction failed");
        exit(1);
    }

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
    printf("Server Started. Waiting for client...\n");

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd == -1) continue;

        pid_t pid = fork();

        if (pid == -1) {
            close(client_fd);
            continue;
        }

        if (pid == 0) {
            close(server_fd);

            char buffer[1024];
            int read_len;
            printf("[Child %d] Connected to client.\n", getpid());

            while((read_len = read(client_fd, buffer, sizeof(buffer))) > 0) {
                write(client_fd, buffer, read_len);
            }

            printf("[Child %d] Client is disconnected.\n", getpid());

            close(client_fd);
            exit(0);
        } else {
            close(client_fd);
        }
    }

    close(server_fd);

    return 0;
}