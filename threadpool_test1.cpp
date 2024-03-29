/*
 * Fork/Join Framework 
 *
 * Test 1.
 *
 * Tests simple task execution.
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
 * A FJ task that adds 2 numbers. 
 */
static void *
adder_task(threadpool *pool, struct arg2 * data)
{
    return (void *)(data->a + data->b);
}

static int
run_test(int nthreads)
{
    struct benchmark_data * bdata = start_benchmark();
    uintptr_t ssum;
    {
        threadpool threadpool(nthreads);
   
        arg2 args = {
                .a = 20,
                .b = 22,
                };

        auto sum = threadpool.submit((fork_join_task_t) adder_task, &args);

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
usage(char *av0, int exvalue)
{
    fprintf(stderr, "Usage: %s [-n <n>]\n"
                    " -n number of threads in pool, default %d\n"
                    , av0, DEFAULT_THREADS);
    exit(exvalue);
}

int 
main(int ac, char *av[]) 
{
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
