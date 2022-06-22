/*
 * Fork/Join Framework 
 *
 * Test 2.
 *
 * Tests running multiple tasks.
 *
 * Written by G. Back for CS3214 Fall 2014.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

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

static int
run_test(int nthreads) {
    bool success = true;
    struct benchmark_data *bdata = start_benchmark();
    {
        threadpool threadpool(nthreads);

#define NTASKS 200
        std::unique_ptr <future> f[NTASKS];
        struct arg2 args[NTASKS];
        for (int i = 0; i < NTASKS; i++) {
            args[i].a = i;
            args[i].b = i + 1;
            f[i] = threadpool.submit((fork_join_task_t) multiplier_task, args + i);
        }

        for (int i = 0; i < NTASKS; i++) {
            uintptr_t sprod = (uintptr_t) f[i]->get();
            if (sprod != i * (i + 1))
                success = false;
        }
    }
    stop_benchmark(bdata);

    // consistency check
    if (!success) {
        fprintf(stderr, "Wrong result\n");
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
