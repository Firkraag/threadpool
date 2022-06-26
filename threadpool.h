#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "memory"
#include "pthread.h"
#include "list.h"

class threadpool;

typedef void *(*fork_join_task_t)(threadpool *pool, void *data);

typedef enum {
    NOT_STARTED = 0,
    IN_PROGRESS = 1,
    COMPLETED = 2
} status_t;
class future_wrapper;
class future : public list_element {
public:
    status_t status = NOT_STARTED;
    void *data;
    fork_join_task_t task;
    void *result;
    threadpool *pool;
    pthread_cond_t done;
    future_wrapper *wrapper;
    void *get();

    future(void *data, fork_join_task_t task, threadpool *pool);

    ~future();
};
class future_wrapper : public list_element {
public:
    std::shared_ptr <future> fut;

    future_wrapper(std::shared_ptr <future> fut) : fut(fut) {
        fut->wrapper = this;
    }
};

class worker {
public:
    pthread_t tid;

    void run(threadpool *pool);

    ~worker();
};


class threadpool {
private:
    worker *workers;
public:
    list global_queue;
    bool shutdown = false;
    pthread_mutex_t lock;

    threadpool(int nthreads);

    std::shared_ptr<future> submit(fork_join_task_t task, void *data);

    ~threadpool();
};


#endif
