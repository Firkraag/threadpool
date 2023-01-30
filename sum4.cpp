#include "iostream"

using namespace std;
#define SIZE 1000000

int main() {
    int array[SIZE];
    for (int i = 0; i < SIZE; ++i) {
        array[i] = i + 1;
    }
    long int sum = 0;
    for (int i : array) {
        sum += i;
    }
    cout << sum << endl;
}