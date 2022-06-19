#include "threadpool.h"

static __thread worker *w = nullptr;

static void *worker_thread(void *args) {
    w = (worker *) args;
    threadpool *pool = w->pool;
    while (true) {
        future *future;
        if (pool->shutdown) {
            break;
        }
        pthread_mutex_lock(&pool->lock);
        if (!w->task_queue.empty()) {
            future = w->task_queue.pop_front();
        } else if (!pool->global_queue.empty()) {
            future = pool->global_queue.pop_front();
        } else {
            future = pool->steal_task();
        }
        if (future == nullptr) {
            pthread_mutex_unlock(&pool->lock);
            continue;
        }
        future->status = IN_PROGRESS;
        pthread_mutex_unlock(&pool->lock);
        future->result = (future->task)(pool, future->data);
        future->status = COMPLETED;
        pthread_cond_signal(&future->done);
    }
    return nullptr;
}

future *threadpool::steal_task() {
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
        pthread_create(&workers[i].tid, NULL, worker_thread, workers + i);
    }
}

future *threadpool::submit(fork_join_task_t task, void *data) {
    future *future = new class future(data, task, this);
    pthread_mutex_lock(&lock);
    (w == nullptr ? global_queue : w->task_queue).push_back(&future->element);
    pthread_mutex_unlock(&lock);
    return future;
}

threadpool::~threadpool() {
    shutdown = true;
    delete[] workers;
    while (!global_queue.empty()) {
        delete global_queue.pop_front();
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