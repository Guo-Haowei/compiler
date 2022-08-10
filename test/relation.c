#include "test.h"

int main()
{
    assert(0, 0 == 1);
    assert(1, 42 == 42);
    assert(1, 0 != 1);
    assert(0, 42 != 42);
    assert(1, 0 < 1);
    assert(0, 1 < 1);
    assert(0, 2 < 1);
    assert(1, 0 <= 1);
    assert(1, 1 <= 1);
    assert(0, 2 <= 1);
    assert(1, 1 > 0);
    assert(0, 1 > 1);
    assert(0, 1 > 2);
    assert(1, 1 >= 0);
    assert(1, 1 >= 1);
    assert(0, 1 >= 2);
    assert(1, 1 >= -2);
    printf("OK\n");
    return 0;
}