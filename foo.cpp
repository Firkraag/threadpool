#include "vector"

using namespace std;
#define SIZE 10000
#define SUBSIZE 5
int arr[SIZE];

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

void foo(void *args) {
    struct args *ptr = (struct args *) args;
    printf("%ld\n", ptr->array - arr);

}

int main() {
    vector<args> args_list;
    for (int i = 0; i < SIZE; ++i) {
        arr[i] = i + 1;
    }

    for (int i = 0; i < SIZE; i += SUBSIZE) {
        args_list.emplace_back(arr + i, SUBSIZE);
        foo(&args_list.back());
    }
}