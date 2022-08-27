#include "test.h"

int main()
{
    {
        int x = 3;
        ASSERT(3, *&x);
    }
    {
        int x = 3, *y = &x, **z = &y;
        ASSERT(3, **z);
    }
    {
        int x = 3, y = 5;
        ASSERT(5, *(&x + 1));
    }
    {
        int x = 3;
        int y = 5;
        ASSERT(3, *(&y - 1));
    }
    {
        int x = 3;
        int y = 5;
        ASSERT(5, *(&x + 1));
        ASSERT(5, *(&x - (-1)));
    }
    {
        int x = 3;
        int* y = &x;
        *y = 5;
        ASSERT(5, x);
    }
    {
        int x = 3;
        int y = 5;
        ASSERT(7, *(&x + 1) = 7);
    }
    {
        int x = 3, y = 5;
        ASSERT(7, *(&y - 2 + 1) = 7);
    }
    {
        int x = 3;
        ASSERT(5, (&x + 2) - &x + 3);
    }
    {
        int x;
        char y;
        int z;
        x = 10;
        y = 20;
        z = 30;
        ASSERT(x, 10);
        ASSERT(y, 20);
        ASSERT(z, 30);
    }
    {
        int x;
        char y;
        int z;
        z = 30;
        y = 20;
        x = 10;
        ASSERT(x, 10);
        ASSERT(y, 20);
        ASSERT(z, 30);
    }

    printf("OK\n");
    return 0;
}