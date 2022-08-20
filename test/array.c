#include "test.h"

int main()
{
    {
        int x[2], *y = x;
        *y = 3;
        ASSERT(3, *x);
    }
    {

        int x[3];
        *x = 3;
        *(x + 1) = 4;
        *(x + 2) = 5;
        ASSERT(3, *x);
        ASSERT(4, *(x + 1));
        ASSERT(5, *(x + 2));
    }
    {
        int x[2][3], *y = x;
        *y = 10;
        *(y + 1) = 1;
        *(y + 2) = 2;
        *(y + 3) = 3;
        *(y + 4) = 4;
        *(y + 5) = 5;

        ASSERT(10, **x);
        ASSERT(1, *(*x + 1));
        ASSERT(2, *(*x + 2));
        ASSERT(3, **(x + 1));
        ASSERT(4, *(*(x + 1) + 1));
        ASSERT(5, *(*(x + 1) + 2));
    }
    {
        int x[3];
        *x = 3;
        x[1] = 4;
        2 [x] = 5;
        ASSERT(3, *x);
        ASSERT(4, *(x + 1));
        ASSERT(5, *(x + 2));
    }
    {
        int x[2][3], *y = x;
        y[0] = 0;
        y[1] = 1;
        y[2] = 2;
        y[3] = 3;
        y[4] = 4;
        y[5] = 5;
        ASSERT(0, x[0][0]);
        ASSERT(1, x[0][1]);
        ASSERT(2, x[0][2]);
        ASSERT(3, x[1][0]);
        ASSERT(4, x[1][1]);
        ASSERT(5, x[1][2]);
    }

    printf("OK\n");
    return 0;
}
