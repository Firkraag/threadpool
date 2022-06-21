#include "threadpool.h"

static void *worker_thread(void *args) {
    threadpool *pool = (threadpool *) args;
    while (true) {
        if (pool->shutdown) {
            break;
        }
        pthread_mutex_lock(&pool->lock);
        if (pool->global_queue.empty()) {
            pthread_mutex_unlock(&pool->lock);
            continue;
        } else {
            future *future = pool->global_queue.pop_front();
            future->status = IN_PROGRESS;
            pthread_mutex_unlock(&pool->lock);
            future->result = (future->task)(pool, future->data);
            future->status = COMPLETED;
            pthread_cond_signal(&future->done);
        }
    }
    return nullptr;
}

void worker::run(threadpool *pool) {
    pthread_create(&tid, NULL, worker_thread, pool);
}

worker::~worker() {
    pthread_join(tid, nullptr);
}

threadpool::threadpool(int nthreads) {
    pthread_mutex_init(&lock, NULL);
    workers = new worker[nthreads];

    for (int i = 0; i < nthreads; i++) {
        workers[i].run(this);
    }
}

std::unique_ptr<future> threadpool::submit(fork_join_task_t task, void *data) {
    future *fut = new class future(data, task, this);
    pthread_mutex_lock(&lock);
    global_queue.push_back(&fut->element);
    pthread_mutex_unlock(&lock);
    return std::unique_ptr<future>(fut);
}

threadpool::~threadpool() {
    shutdown = true;
    delete[] workers;
    while (!global_queue.empty()) {
        delete global_queue.pop_front();
    }
    pthread_mutex_destroy(&lock);
}

future::future(void *data, fork_join_task_t task, threadpool *pool) : data(data), task(task), pool(pool) {
    pthread_cond_init(&done, nullptr);
}

future::~future() {
    pthread_cond_destroy(&done);
}

void *future::get() {
    pthread_mutex_lock(&pool->lock);
    if (status == NOT_STARTED) {
        element.remove();
        status = IN_PROGRESS;
        pthread_mutex_unlock(&pool->lock);
        result = task(pool, data);
        status = COMPLETED;
        pthread_cond_signal(&done);
    } else {
        while (status != COMPLETED) {
            pthread_cond_wait(&done, &pool->lock);
        }
        pthread_mutex_unlock(&pool->lock);
    }
    return result;
}