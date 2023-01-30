#include "threadpool.h"
#include <cassert>
#include <pthread.h>

static void *worker_thread(void *args) {
    auto *pool = (threadpool *) args;
    while (true) {
        pthread_mutex_lock(&pool->lock);
        while (!pool->shutdown && pool->global_queue.empty()) {
            pthread_cond_wait(&pool->has_task, &pool->lock);
        }
        if (pool->shutdown) {
          pthread_mutex_unlock(&pool->lock);
          break;
        }
        auto *fut = (future *) pool->global_queue.pop_front();
        fut->status = IN_PROGRESS;
        pthread_mutex_unlock(&pool->lock);
        auto *result  = (fut->task)(pool, fut->data);
        pthread_mutex_lock(&pool->lock);
        fut->result = result;
        fut->status = COMPLETED;
        pthread_cond_signal(&fut->done);
        pthread_mutex_unlock(&pool->lock);
    }
    return nullptr;
}

void worker::run(threadpool *pool) {
    pthread_create(&tid, nullptr, worker_thread, pool);
}

worker::~worker() {
    pthread_join(tid, nullptr);
}

threadpool::threadpool(int nthreads) {
    pthread_mutex_init(&lock, nullptr);
    pthread_cond_init(&has_task, nullptr);
    workers = new worker[nthreads];

    for (int i = 0; i < nthreads; i++) {
        workers[i].run(this);
    }
}

std::unique_ptr<future> threadpool::submit(fork_join_task_t task, void *data) {
    auto fut = new future(data, task, this);
    pthread_mutex_lock(&lock);
    global_queue.push_back(fut);
    pthread_cond_signal(&has_task);
    pthread_mutex_unlock(&lock);
    return std::unique_ptr<future>(fut);
}

threadpool::~threadpool() {
    assert(global_queue.empty());
    shutdown = true;
    pthread_cond_broadcast(&has_task);
    delete[] workers;
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&has_task);
}

future::future(void *data, fork_join_task_t task, threadpool *pool) : data(data), task(task), pool(pool) {
    pthread_cond_init(&done, nullptr);
}

future::~future() {
    pthread_mutex_lock(&pool->lock);
    if (status == NOT_STARTED) {
        remove();
        pthread_mutex_unlock(&pool->lock);
    } else {
        while (status != COMPLETED) {
            pthread_cond_wait(&done, &pool->lock);
        }
        pthread_mutex_unlock(&pool->lock);
    }
    pthread_cond_destroy(&done);
}

void *future::get() {
    pthread_mutex_lock(&pool->lock);
    if (status == NOT_STARTED) {
        remove();
        status = IN_PROGRESS;
        pthread_mutex_unlock(&pool->lock);
        auto *result = task(pool, data);
        pthread_mutex_lock(&pool->lock);
        this->result = result;
        status = COMPLETED;
        pthread_cond_signal(&done);
        pthread_mutex_unlock(&pool->lock);
    } else {
        while (status != COMPLETED) {
            pthread_cond_wait(&done, &pool->lock);
        }
        pthread_mutex_unlock(&pool->lock);
    }
    return result;
}