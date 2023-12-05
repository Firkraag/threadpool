#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "list.h"
#include "memory"
#include "pthread.h"

class threadpool;

typedef void *(*fork_join_task_t)(threadpool *pool, void *data);

typedef enum { NOT_STARTED, IN_PROGRESS, COMPLETED } status_t;

class future : public list_element {
public:
  status_t status = NOT_STARTED;
  void *data;
  fork_join_task_t task;
  void *result;
  threadpool *pool;
  pthread_cond_t done;

  void *get();

  future(void *data, fork_join_task_t task, threadpool *pool);

  ~future();
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
  pthread_cond_t has_task;

  explicit threadpool(int nthreads);

  std::unique_ptr<future> submit(fork_join_task_t task, void *data);

  ~threadpool();
};

#endif
