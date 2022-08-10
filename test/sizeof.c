#include "test.h"

int main()
{
    {
        int x;
        assert(4, sizeof(x));
    }
    {
        int x;
        assert(4, sizeof x);
    }
    {
        int *x;
        assert(8, sizeof(x));
    }
    {
        int x[4];
        assert(16, sizeof(x));
    }
    {
        int x[3][4];
        assert(48, sizeof(x));
    }
    {
        int x[3][4];
        assert(16, sizeof(*x));
    }
    {
        int x[3][4];
        assert(4, sizeof(**x));
    }
    {
        int x[3][4];
        assert(5, sizeof(**x) + 1);
    }
    {
        int x[3][4];
        assert(5, sizeof **x + 1);
    }
    {
        int x[3][4];
        assert(4, sizeof(**x + 1));
    }
    {
        int x = 1;
        assert(4, sizeof(x = 2));
    }
    {
        int x = 1;
        sizeof(x = 2);
        assert(1, x);
    }

    printf("OK\n");
    return 0;
}