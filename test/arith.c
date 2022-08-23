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
    ASSERT(0, 1073741824 * 100 / 100);

    {
        int i = 2;
        ASSERT(7, i += 5);
    }
    {
        int i = 2;
        i += 5;
        ASSERT(7, i);
    }
    {
        int i = 5;
        ASSERT(3, i -= 2);
    }
    {
        int i = 5;
        i -= 2;
        ASSERT(3, i);
    }
    {
        int i = 3;
        ASSERT(6, i *= 2);
    }
    {
        int i = 3;
        i *= 2;
        ASSERT(6, i);
    }
    {
        int i = 12;
        ASSERT(4, i /= 3);
    }
    {
        int i = 12;
        i /= 3;
        ASSERT(4, i);
    }

    printf("OK\n");
    return 0;
}