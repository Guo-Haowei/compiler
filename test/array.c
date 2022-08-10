#include "test.h"

int main()
{
    {
        int x[2], *y = x;
        *y = 3;
        assert(3, *x);
    }
    {

        int x[3];
        *x=3;
        *(x+1)=4;
        *(x+2)=5;
        assert(3, *x);
        assert(4, *(x + 1));
        assert(5, *(x + 2));
    }
    {
        int x[2][3], *y = x;
        *y = 10;
        *(y + 1) = 1;
        *(y + 2) = 2;
        *(y + 3) = 3;
        *(y + 4) = 4;
        *(y + 5) = 5;

        assert(10, **x);
        assert(1, *(*x + 1));
        assert(2, *(*x + 2));
        assert(3, **(x + 1));
        assert(4, *(*(x + 1) + 1));
        assert(5, *(*(x + 1) + 2));
    }
    {
        int x[3];
        *x = 3;
        x[1] = 4;
        2[x] = 5;
        assert(3, *x);
        assert(4, *(x + 1));
        assert(5, *(x + 2));
    }
    {
        int x[2][3], *y = x;
        y[0] = 0;
        y[1] = 1;
        y[2] = 2;
        y[3] = 3;
        y[4] = 4;
        y[5] = 5;
        assert(0, x[0][0]);
        assert(1, x[0][1]);
        assert(2, x[0][2]);
        assert(3, x[1][0]);
        assert(4, x[1][1]);
        assert(5, x[1][2]);
    }

    printf("OK\n");
    return 0;
}
