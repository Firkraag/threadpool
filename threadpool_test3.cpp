/*
 * Fork/Join Framework 
 *
 * Test 3.
 *
 * Tests simple recursive join.
 *
 * Written by G. Back for CS3214 Fall 2014.
 */
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <ctime>

#include "threadpool.h"
#include "threadpool_lib.h"

#define DEFAULT_THREADS 1

/* Data to be passed to callable. */
struct arg2 {
    uintptr_t a;
    uintptr_t b;
};

/* 
 * A FJ task that multiplies 2 numbers. 
 */
static void *
multiplier_task(threadpool *pool, struct arg2 *data) {
    return (void *) (data->a * data->b);
}

/* 
 * A FJ task that adds 2 numbers. 
 */
static void *
adder_task(threadpool *pool, struct arg2 *data) {
    return (void *) (data->a + data->b);
}

static void *
test_task(threadpool *pool, struct arg2 *data) {
    auto f1 = pool->submit((fork_join_task_t) adder_task, data);
    auto r1 = (uintptr_t) f1->get();

    struct arg2 a2 = {
            .a = r1,
            .b = 7,
    };
    auto f2 = pool->submit((fork_join_task_t) multiplier_task, &a2);
    auto r2 = (uintptr_t) f2->get();
    return (void *) r2;
}

static int
run_test(int nthreads) {
    uintptr_t ssum;
    struct benchmark_data *bdata = start_benchmark();
    {
        threadpool threadpool(nthreads);

        struct arg2 args = {
                .a = 2,
                .b = 4,
        };

        std::unique_ptr<future> sum = threadpool.submit((fork_join_task_t) test_task, &args);

        ssum = (uintptr_t) sum->get();
    }
    stop_benchmark(bdata);

    // consistency check
    if (ssum != 42) {
        fprintf(stderr, "Wrong result, expected 42, got %ld\n", ssum);
        abort();
    }

    report_benchmark_results(bdata);
    printf("Test successful.\n");
    free(bdata);
    return 0;
}

/**********************************************************************************/

static void
usage(char *av0, int exvalue) {
    fprintf(stderr, "Usage: %s [-n <n>]\n"
                    " -n number of threads in pool, default %d\n", av0, DEFAULT_THREADS);
    exit(exvalue);
}

int
main(int ac, char *av[]) {
    int c, nthreads = DEFAULT_THREADS;
    while ((c = getopt(ac, av, "hn:")) != EOF) {
        switch (c) {
            case 'n':
                nthreads = atoi(optarg);
                break;
            case 'h':
                usage(av[0], EXIT_SUCCESS);
        }
    }

    return run_test(nthreads);
}
