CC=g++
CXX=g++
# Note that this defines NDEBUG, which removes all assert()s from your code
CXXFLAGS=-Wall -Werror -Wmissing-prototypes -DNDEBUG -O3 -std=c++17
LDFLAGS=-pthread
# for debugging, you may use these 2
#LDFLAGS=-pthread -fsanitize=undefined
#CFLAGS=-Wall -O0 -g -Werror -Wmissing-prototypes -fopenmp -fsanitize=undefined

OBJ=threadpool.o list.o threadpool_lib.o

#ALL=quicksort psum_test fib_test mergesort nqueens \
#	threadpool_test threadpool_test2 threadpool_test3 threadpool_test4 threadpool_test5 \
#	threadpool_test6
ALL=threadpool_test1 threadpool_test2 threadpool_test3 threadpool_test4 threadpool_test5  threadpool_test6

all: $(ALL)

threadpool_test6: threadpool_test6.o $(OBJ)

threadpool_test5: threadpool_test5.o $(OBJ)

threadpool_test4: threadpool_test4.o $(OBJ)

threadpool_test3: threadpool_test3.o $(OBJ)

threadpool_test2: threadpool_test2.o $(OBJ)

threadpool_test1: threadpool_test1.o $(OBJ)

test: $(ALL)
	for executable in $(ALL) ; do ./$$executable; done
#quicksort: quicksort.o $(OBJ)
#
#nqueens: nqueens.o $(OBJ)
#
#mergesort: mergesort.o $(OBJ)
#
#psum_test: psum_test.o $(OBJ)
#
#fib_test: fib_test.o $(OBJ)

clean:
	rm -f *.o $(ALL) *.json

