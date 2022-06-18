//
// Created by wq on 2022/6/15.
//

#include "threadpool.h"

static __thread worker *w = nullptr;

static void *worker_thread(void *args) {
    threadpool *pool;

    w = (worker *) args;
    pool = w->pool;
    while (true) {
        list_element *element;
        future *future;
        if (pool->shutdown) {
            break;
        }
        pthread_mutex_lock(&pool->lock);
        if (!w->task_queue.empty()) {
            element = w->task_queue.pop_front();
        } else if (!pool->global_queue.empty()) {
            element = pool->global_queue.pop_front();
        } else {
            element = pool->steal_task();
        }
        if (element == nullptr) {
            pthread_mutex_unlock(&pool->lock);
            continue;
        }
//        future = list<class future, offsetof(class future, element)>::entry(element);
        future = list_entry(element, class future, element);
        future->status = IN_PROGRESS;
        pthread_mutex_unlock(&pool->lock);
        future->result = (future->task)(pool, future->data);
        future->status = COMPLETED;
        pthread_cond_signal(&future->done);
    }
    return nullptr;
}

list_element *threadpool::steal_task() {
    for (int i = 0; i < nthreads; ++i) {
        if (!workers[i].task_queue.empty()) {
            return workers[i].task_queue.pop_front();
        }
    }
    return nullptr;
}

threadpool::threadpool(int nthreads) {
    this->nthreads = nthreads;
    pthread_mutex_init(&lock, NULL);
    workers = new worker[nthreads];

    for (int i = 0; i < nthreads; i++) {
        workers[i].pool = this;
    }
    for (int i = 0; i < nthreads; i++) {
        pthread_create(&workers[i].tid, NULL, worker_thread, workers + i);
    }
}

future *threadpool::submit(fork_join_task_t task, void *data) {
    future *future = new class future(data, task, this);
    pthread_mutex_lock(&lock);
    if (w == nullptr) {
        global_queue.push_back(&future->element);
    } else {
        w->task_queue.push_back(&future->element);
    }
    pthread_mutex_unlock(&lock);
    return future;
}

threadpool::~threadpool() {
    shutdown = true;
    delete[] workers;
    while (!global_queue.empty()) {
        list_element *element = global_queue.pop_front();
        future *future = list_entry(element, class future, element);
        delete future;
    }
    pthread_mutex_destroy(&lock);
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