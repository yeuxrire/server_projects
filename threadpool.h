#ifndef THREADPOOL_H
#define THREADPOOL_H

void init_thread_pool();

void pool_submit(int client_fd);

void shutdown_thread_pool();

#endif