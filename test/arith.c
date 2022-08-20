#include "test.h"

int main()
{
    ASSERT(0, 0);
    ASSERT(42, 42);
    ASSERT(21, 5 + 20 - 4);
    ASSERT(41, 12 + 34 - 5);
    ASSERT(47, 5 + 6 * 7);
    ASSERT(15, 5 * (9 - 6));
    ASSERT(4, (3 + 5) / 2);
    ASSERT(10, -10 + 20);
    ASSERT(-10, -10);
    ASSERT(3, (1, 2, 3));

    printf("OK\n");
    return 0;
}