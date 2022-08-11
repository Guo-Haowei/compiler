#include "test.h"

int main()
{
    {
        int x = 3;
        assert(3, *&x);
    }
    {
        int x=3, *y=&x, **z=&y;
        assert(3, **z);
    }
    {
        int x=3, y=5;
        assert(5, *(&x+1));
    }
    {
        int x=3;
        int y=5;
        assert(3, *(&y-1));
    }
    {
        int x=3;
        int y=5;
        assert(5, *(&x-(-1)));
    }
    {
        int x=3;
        int* y=&x;
        *y=5;
        assert(5, x);
    }
    {
        int x=3;
        int y=5;
        assert(7, *(&x+1)=7);
    }
    {
        int x=3, y=5;
        assert(7, *(&y-2+1)=7);
    }
    {
        int x=3;
        assert(5, (&x+2)-&x+3);
    }
    {
        int x; char y; int z;
        x = 10;
        y = 20;
        z = 30;
        assert(x, 10);
        assert(y, 20);
        assert(z, 30);
    }
    {
        int x; char y; int z;
        z = 30;
        y = 20;
        x = 10;
        assert(x, 10);
        assert(y, 20);
        assert(z, 30);
    }

    printf("OK\n");
    return 0;
}