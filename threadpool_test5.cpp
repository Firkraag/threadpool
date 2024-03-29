/*
 * Fork/Join Framework 
 *
 * Test that the pool keeps processing tasks even in the absence of a call to
 * future_get by external threads.  (Filters out implementations that do not have
 * a correct worker implementation that keeps checking the global queue.)
 *
 * Written by G. Back for CS3214 Spring 2016.
 */
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/resource.h>
#include <ctime>

#include "threadpool.h"
#include "threadpool_lib.h"
#define DEFAULT_THREADS 2
#define DEFAULT_TASKS 40

struct time_range {
    uintptr_t taskno;
    struct timeval start;
    struct timeval end;
};

static int count;

static void *
test_task(threadpool *pool, struct time_range * data)
{
    __sync_fetch_and_add(&count, 1);
    return (void *)data->taskno;
}

static int
run_test(int nthreads, int ntasks)
{
    struct benchmark_data * bdata = start_benchmark();
    struct time_range start_end[ntasks];
    {
        threadpool threadpool(nthreads);
   
        std::unique_ptr<future> futures[ntasks];

        printf("starting %d tasks...\n", ntasks);
        count = 0;
        for (int i = 0; i < ntasks; i++) {
            start_end[i].taskno = i;
            futures[i] = threadpool.submit((fork_join_task_t) test_task, start_end + i);
        }

        struct timespec sleep_time = {
                .tv_sec = 0,
                .tv_nsec = 200*1000*1000 // 200 ms should be plenty of time for those tasks to be
                        // processed.
        };
        nanosleep(&sleep_time, nullptr);
        if (count != ntasks) {
            fprintf(stderr, "Your pool did not make progress in the absence of calls to future_get\n");
            abort();
        }

        for (int i = 0; i < ntasks; i++) {
            auto r = (uintptr_t) futures[i]->get();
            // consistency check
            if (r != i) {
                fprintf(stderr, "Wrong result, expected %d, got %lu\n", i, r);
                abort();
            }
        }

    }
    stop_benchmark(bdata);

    report_benchmark_results(bdata);
    printf("Test successful.\n");
    free(bdata);
    return 0;
}

/**********************************************************************************/

static void
usage(char *av0, int exvalue)
{
    fprintf(stderr, "Usage: %s [-n <n>] [-t <t>]\n"
                    " -n number of threads in pool, default %d\n"
                    " -t number of tasks, default %d\n"
                    , av0, DEFAULT_THREADS, DEFAULT_TASKS);
    exit(exvalue);
}

int 
main(int ac, char *av[]) 
{
    int c, nthreads = DEFAULT_THREADS, ntasks = DEFAULT_TASKS;
    while ((c = getopt(ac, av, "hn:t:")) != EOF) {
        switch (c) {
        case 'n':
            nthreads = atoi(optarg);
            if (nthreads == 1) {
                fprintf(stderr, "Cannot test for concurrency with only 1 thread\n");
                abort();
            }
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
