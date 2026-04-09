#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "threadpool.h"
#include "http_parser.h"

#define THREAD_POOL_SIZE 8

typedef struct Task {
    int client_fd;
    struct Task *next;
} Task;

typedef struct {
    Task *front;
    Task *rear;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t threads[THREAD_POOL_SIZE];
    int shutdown;
} ThreadPool;

ThreadPool pool;

void *worker_thread(void *arg) {
    while(1) {
        pthread_mutex_lock(&pool.lock);

        while(pool.count == 0 && !pool.shutdown) {
            pthread_cond_wait(&pool.notify, &pool.lock);
        }

        if (pool.shutdown && pool.count == 0) {
            pthread_mutex_unlock(&pool.lock);
            break;
        }

        Task *task = pool.front;
        pool.front = task->next;

        if (pool.front == NULL) {
            pool.rear = NULL;
        }
        pool.count--;

        pthread_mutex_unlock(&pool.lock);

        printf("[Worker %lu] Processing request (FD: %d)\n", pthread_self(), task->client_fd);
        handle_http_request(task->client_fd);
        close(task->client_fd);
        free(task);
    }

    printf("[Worker %lu] Safely terminated.\n", pthread_self());
    pthread_exit(NULL);
}

void init_thread_pool() {
    pool.front = pool.rear = NULL;
    pool.count = 0;
    pool.shutdown = 0;
    pthread_mutex_init(&pool.lock, NULL);
    pthread_cond_init(&pool.notify, NULL);

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_create(&pool.threads[i], NULL, worker_thread, NULL);
    }

    printf("[Thread Pool] Initialized. [%d] workers are standing by...\n", THREAD_POOL_SIZE);
}

void pool_submit(int client_fd) {
    Task *new_task = malloc(sizeof(Task));
    new_task->client_fd = client_fd;
    new_task->next = NULL;

    pthread_mutex_lock(&pool.lock);

    if (pool.rear == NULL) {
        pool.front = new_task;
        pool.rear = new_task;
    } else {
        pool.rear->next = new_task;
        pool.rear = new_task;
    }
    pool.count++;

    pthread_cond_signal(&pool.notify);
    pthread_mutex_unlock(&pool.lock);
}

void shutdown_thread_pool() {
    pthread_mutex_lock(&pool.lock);
    pool.shutdown = 1;
    pthread_cond_broadcast(&pool.notify);
    pthread_mutex_unlock(&pool.lock);

    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        pthread_join(pool.threads[i], NULL);
    }

    pthread_mutex_destroy(&pool.lock);
    pthread_cond_destroy(&pool.notify);
    printf("[Thread Pool] All workers successfully terminated.\n");
}