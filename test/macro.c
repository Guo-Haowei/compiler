#include "test.h"

int return_a_plus_one() {
#include "include1.h"
    return a + 1;
}

int main() {
#include "include1.h"
    assert(19, a);
    assert(20, return_a_plus_one());

    printf("OK\n");
    return 0;
}