#include "test.h"

int main()
{
    ASSERT(0, 0 == 1);
    ASSERT(1, 42 == 42);
    ASSERT(1, 0 != 1);
    ASSERT(0, 42 != 42);
    ASSERT(1, 0 < 1);
    ASSERT(0, 1 < 1);
    ASSERT(0, 2 < 1);
    ASSERT(1, 0 <= 1);
    ASSERT(1, 1 <= 1);
    ASSERT(0, 2 <= 1);
    ASSERT(1, 1 > 0);
    ASSERT(0, 1 > 1);
    ASSERT(0, 1 > 2);
    ASSERT(1, 1 >= 0);
    ASSERT(1, 1 >= 1);
    ASSERT(0, 1 >= 2);
    ASSERT(1, 1 >= -2);
    printf("OK\n");
    return 0;
}