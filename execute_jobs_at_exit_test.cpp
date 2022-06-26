/* Test whether threadpool will execute remaining jobs at exit */

#include "threadpool.h"
#include "unistd.h"

#define DEFAULT_THREADS 1

int num = 1;

// job1 will block the worker thread using sleep, thus we can test whether threadpool destructor executes job2.
static void *job1(threadpool *pool, void *args) {
    sleep(3);
    return nullptr;
}

static void *job2(threadpool *pool, void *args) {
    num = 10;
    return nullptr;
}

static int run_test(int nthreads) {
    {
        threadpool threadpool(nthreads);

        threadpool.submit((fork_join_task_t) job1, nullptr);
        threadpool.submit((fork_join_task_t) job2, nullptr);
    }
    if (num != 10) {
        fprintf(stderr, "Wrong result, expected 10, got %d\n", num);
        abort();
    } else {
        printf("Test successful.\n");
    }
    return 0;
}

int main() {
    return run_test(DEFAULT_THREADS);
}
