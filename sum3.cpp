#include "threadpool.h"
#include "iostream"

using namespace std;
#define SIZE 10000
#define SUBSIZE 5

struct args {
    int *array;
    int size;
};

static void *sum(threadpool *pool, void *args) {
    int *array = ((struct args *) args)->array;
    int size = ((struct args *) args)->size;
    if (size < SUBSIZE) {
        int *sum = new int(0);
        for (int i = 0; i < size; ++i) {
            *sum += array[i];
        }
        return sum;
    } else {
        struct args args1{array, size / 2};
        struct args args2{array + size / 2, size - size / 2};
        auto fut1 = pool->submit(sum, &args1);
        unique_ptr<int> ptr1((int *) fut1->get());
        unique_ptr<int> ptr2((int *) sum(pool, &args2));
        return new int(*ptr1 + *ptr2);
    }
}

int main() {
    int array[SIZE];
    for (int i = 0; i < SIZE; ++i) {
        array[i] = i + 1;
    }
    struct args args{array, SIZE};
    threadpool pool(8);
    auto fut = pool.submit(sum, &args);
    int *ptr = (int *) fut->get();
    cout << *ptr << endl;
    delete ptr;
}