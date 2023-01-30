/* To test whether a future object get deallocated twice
 * If it is deallocated twice, this program will abort and
 * an error message will occur
 */
#include "threadpool.h"
#include "unistd.h"

#define DEFAULT_THREADS 1

/* Data to be passed to callable. */
struct arg2 {
    uintptr_t a;
    uintptr_t b;
};

/* 
 * A FJ task that adds 2 numbers. 
 */
static void *adder_task(threadpool *pool, struct arg2 *data) {
    sleep(2);
    return (void *) (data->a + data->b);
}

static int run_test(int nthreads) {
    threadpool threadpool(nthreads);

    arg2 args = {
            .a = 20,
            .b = 22,
    };

    auto fut = threadpool.submit((fork_join_task_t) adder_task, &args);
    auto sum = (uintptr_t) fut->get();
    printf("sum = %lu\n", sum);
    printf("Test successful.\n");
    return 0;
}

int main() {
    return run_test(DEFAULT_THREADS);
}
