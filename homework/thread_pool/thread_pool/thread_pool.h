#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>

#ifndef __THREADPOOL_H_
#define __THREADPOOL_H_

typedef struct thread_pool_t thread_pool_t;
typedef struct thread_task_t thread_task_t;

thread_pool_t *thread_pool_create(int thread_num, int max_queue_num);

int thread_pool_add(thread_pool_t *thread_pool, void *(*function)(void *), void *arg);

int thread_pool_destroy(thread_pool_t *pool);

void *thread_function(void *arg);

#endif
