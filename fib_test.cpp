/*
 * Parallel fibonacci.
 *
 * This is just a toy program to see how well the
 * underlying FJ framework supports extremely fine-grained
 * tasks.
 *
 * Written by G. Back for CS3214 Fall 2014.
 */
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>

#include "thread_pool.h"
#include "threadpool_lib.h"

/* Data to be passed to callable. */
struct problem_parameters {
    unsigned n;
};

static void *
fibonacci(thread_pool *pool, void *_data) {
    struct problem_parameters *p = (struct problem_parameters *) _data;
    if (p->n <= 1)
        return (void *) 1;

    struct problem_parameters left_half = {.n = p->n - 1};
    struct problem_parameters right_half = {.n = p->n - 2};
    future *f = pool->submit((fork_join_task_t) fibonacci, &right_half);
    uintptr_t lresult = (uintptr_t) fibonacci(pool, &left_half);
    uintptr_t rresult = (uintptr_t) f->get();
    delete f;
    return (void *) (lresult + rresult);
}

static void usage(char *av0, int nthreads) {
    fprintf(stderr, "Usage: %s [-d <n>] [-n <n>] <N>\n"
                    " -n        number of threads in pool, default %d\n", av0, nthreads);
    exit(0);
}

int
main(int ac, char *av[]) {
    int nthreads = 4;
    int c;
    while ((c = getopt(ac, av, "n:h")) != EOF) {
        switch (c) {
            case 'n':
                nthreads = atoi(optarg);
                break;
            case 'h':
                usage(av[0], nthreads);
        }
    }
    if (optind == ac)
        usage(av[0], nthreads);

    int n = atoi(av[optind]);
    thread_pool pool(nthreads);

    struct problem_parameters roottask = {.n = (unsigned) n};

    unsigned long long F[n + 1];
    F[0] = F[1] = 1;
    int i;
    for (i = 2; i < n + 1; i++) {
        F[i] = F[i - 1] + F[i - 2];
    }

    printf("starting...\n");
    struct benchmark_data *bdata = start_benchmark();
    future *f = pool.submit((fork_join_task_t) fibonacci, &roottask);
    unsigned long long Fvalue = (unsigned long long) f->get();
    stop_benchmark(bdata);
    delete f;
    if (Fvalue != F[n]) {
        printf("result %lld should be %lld\n", Fvalue, F[n]);
        abort();
    } else {
        printf("result ok.\n");
        report_benchmark_results(bdata);
    }

    free(bdata);
    return 0;
}

