CC=g++
CXX=g++
# Note that this defines NDEBUG, which removes all assert()s from your code
CXXFLAGS=-Wall -Werror -Wmissing-prototypes -DNDEBUG -O3 -std=c++17
LDFLAGS=-pthread
# for debugging, you may use these 2
#LDFLAGS=-pthread -fsanitize=undefined
#CFLAGS=-Wall -O0 -g -Werror -Wmissing-prototypes -fopenmp -fsanitize=undefined

OBJS=threadpool.o threadpool_lib.o

#ALL=quicksort psum_test fib_test mergesort nqueens \
#	threadpool_test threadpool_test2 threadpool_test3 threadpool_test4 threadpool_test5 \
#	threadpool_test6
ALL=threadpool_test1 threadpool_test2 threadpool_test3 threadpool_test4 threadpool_test5  threadpool_test6 future_deallocation_test\
	execute_jobs_at_exit_test

all: $(ALL)

threadpool_test6: threadpool_test6.o $(OBJS)

threadpool_test5: threadpool_test5.o $(OBJS)

threadpool_test4: threadpool_test4.o $(OBJS)

threadpool_test3: threadpool_test3.o $(OBJS)

threadpool_test2: threadpool_test2.o $(OBJS)

threadpool_test1: threadpool_test1.o $(OBJS)

fib_test: fib_test.o $(OBJS)

future_deallocation_test: future_deallocation_test.o $(OBJS)

execute_jobs_at_exit_test: execute_jobs_at_exit_test.o $(OBJS)

test: $(ALL)
	for executable in $(ALL) ; do ./$$executable; done
#quicksort: quicksort.o $(OBJS)
#
#nqueens: nqueens.o $(OBJS)
#
#mergesort: mergesort.o $(OBJS)
#
#psum_test: psum_test.o $(OBJS)
#
#fib_test: fib_test.o $(OBJS)

clean:
	rm -f *.o $(ALL) *.json

