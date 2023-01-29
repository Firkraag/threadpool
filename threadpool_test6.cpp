/*
 * Fork/Join Framework 
 *
 * Test that future_free() is implemented and working.
 * For this test, we expect that any memory allocated by your
 * threadpool implementation is freed when the program exits.
 *
 * While it is normally ok to not free memory before exiting,
 * libraries that are intended for long-running programs, such
 * as the threadpool library in this project, cannot leak
 * memory during normal operation.
 *
 * Thus, this test will succeed only if valgrind reports this:
 *
 * ==2336955== LEAK SUMMARY:
 * ==2336955==    definitely lost: 0 bytes in 0 blocks
 * ==2336955==    indirectly lost: 0 bytes in 0 blocks
 * ==2336955==      possibly lost: 0 bytes in 0 blocks
 * ==2336955==    still reachable: 0 bytes in 0 blocks
 *
 * Written by G. Back for CS3214 Summer 2020.
 * Updated Fall 2020.
 */
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <ctime>

#include "threadpool.h"
#include "threadpool_lib.h"

#define DEFAULT_THREADS 2
#define DEFAULT_TASKS 50

struct taskno_wrapper {
    uintptr_t taskno;
};

static void *
inner_task(threadpool *pool, struct taskno_wrapper *data) {
    return (void *) data->taskno;
}

static void *
test_task(threadpool *pool, struct taskno_wrapper *data) {
    struct taskno_wrapper childdata = {.taskno = 2 * data->taskno};
    auto child = pool->submit((fork_join_task_t) inner_task, &childdata);
    void *result = child->get();
    if ((uintptr_t) result != 2 * data->taskno)
        abort();
    return (void *) data->taskno;
}

static int
run_test(int nthreads, int ntasks) {
    struct benchmark_data *bdata = start_benchmark();
    {
        threadpool threadpool(nthreads);
        auto *task_data = (struct taskno_wrapper *) malloc(sizeof(struct taskno_wrapper) * ntasks);
//        future **futures = (future **) malloc(sizeof(*futures) * ntasks);
        std::unique_ptr<future> futures[ntasks];
        printf("starting %d tasks...\n", ntasks);
        for (int i = 0; i < ntasks; i++) {
            task_data[i].taskno = i;
            futures[i] = threadpool.submit((fork_join_task_t) test_task, task_data + i);
        }

        for (int i = 0; i < ntasks; i++) {
            auto r = (uintptr_t) futures[i]->get();
            // consistency check
            if (r != i) {
                fprintf(stderr, "Wrong result, expected %d, got %lu\n", i, r);
                abort();
            }
//            delete futures[i];
        }
        free(task_data);
//        free(futures);

    }
    stop_benchmark(bdata);

    report_benchmark_results(bdata);
    printf("Test successful.\n");
    free(bdata);
    return 0;
}

/**********************************************************************************/

static void
usage(char *av0, int exvalue) {
    fprintf(stderr, "Usage: %s [-n <n>] [-t <t>]\n"
                    " -n number of threads in pool, default %d\n"
                    " -t number of tasks, default %d\n", av0, DEFAULT_THREADS, DEFAULT_TASKS);
    exit(exvalue);
}

int
main(int ac, char *av[]) {
    int c, nthreads = DEFAULT_THREADS, ntasks = DEFAULT_TASKS;
    while ((c = getopt(ac, av, "hn:t:")) != EOF) {
        switch (c) {
            case 'n':
                nthreads = atoi(optarg);
                break;
            case 't':
                ntasks = atoi(optarg);
                break;
            case 'h':
                usage(av[0], EXIT_SUCCESS);
        }
    }

    return run_test(nthreads, ntasks);
}
