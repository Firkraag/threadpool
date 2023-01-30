#include "threadpool.h"
#include "string"
#include "iostream"
#include "vector"

using namespace std;
#define SIZE 10000
#define SUBSIZE 5

struct args {
    int *array;
    int size;

    args(int *array, int size) : array(array), size(size) {}
//    args(const args &a) {
//        array = a.array;
//        size = a.size;
//    }
//    args operator=(const args &a) {
//        if (&a != this) {
//            return args(a);
//        }
//        return *this;
//    }
};

static void *sum(threadpool *pool, void *args) {
    int *array = ((struct args *) args)->array;
    int size = ((struct args *) args)->size;
    delete (struct args *) args;
    int *sum = new int(0);
    for (int i = 0; i < size; ++i) {
        *sum += array[i];
    }
    return sum;
}

int main() {
    int array[SIZE];
    for (int i = 0; i < SIZE; ++i) {
        array[i] = i + 1;
    }
    vector<unique_ptr<future>> future_list;
    threadpool pool(8);
//    printf("abc\n");
    for (int i = 0; i < SIZE; i += SUBSIZE) {
//        printf("i = %d\n", i);
//        printf("before future list\n");
        future_list.push_back(pool.submit(sum, new args(array + i, SUBSIZE)));
    }
    long long int sum = 0;
//    printf("abc\n");
    for (const auto &fut: future_list) {
        int *ptr = (int *) fut->get();
        sum += *ptr;
        delete ptr;
    }
    cout << sum << endl;
}