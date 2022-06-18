#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "pthread.h"
#include "list.h"

class threadpool;

typedef void *(*fork_join_task_t)(threadpool *pool, void *data);

typedef enum {
    NOT_STARTED = 0,
    IN_PROGRESS = 1,
    COMPLETED = 2
} status_t;

class future {
public:
    list_element element;
    status_t status = NOT_STARTED;
    void *data;
    fork_join_task_t task;
    void *result;
    threadpool *pool;
    pthread_cond_t done;

    void *get();
    future(void *data, fork_join_task_t task, threadpool *pool) : data(data), task(task), pool(pool) {
        pthread_cond_init(&done, nullptr);
    }
    ~future() {
        pthread_cond_destroy(&done);
    }
};

class worker {
public:
    list task_queue;
    pthread_t tid;
    threadpool *pool;
    ~worker() {
        pthread_join(tid, nullptr);
        while (!task_queue.empty())
        {
            list_element *element = task_queue.pop_front();
            future *future = list_entry(element, class future, element);
            delete future;
        }
    }
};

class threadpool {
private:
    int nthreads;
    worker *workers;
public:
    list global_queue;
    bool shutdown = false;
    pthread_mutex_t lock;

    threadpool(int nthreads);
    future *submit(fork_join_task_t task, void *data);
    list_element *steal_task();
    ~threadpool();
};


#endif
