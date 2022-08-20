#include "test.h"

int main()
{
    {
        int x;
        ASSERT(4, sizeof(x));
    }
    {
        int x;
        ASSERT(4, sizeof x);
    }
    {
        int* x;
        ASSERT(8, sizeof(x));
    }
    {
        int x[4];
        ASSERT(16, sizeof(x));
    }
    {
        int x[3][4];
        ASSERT(48, sizeof(x));
    }
    {
        int x[3][4];
        ASSERT(16, sizeof(*x));
    }
    {
        int x[3][4];
        ASSERT(4, sizeof(**x));
    }
    {
        int x[3][4];
        ASSERT(5, sizeof(**x) + 1);
    }
    {
        int x[3][4];
        ASSERT(5, sizeof **x + 1);
    }
    {
        int x[3][4];
        ASSERT(4, sizeof(**x + 1));
    }
    {
        int x = 1;
        ASSERT(4, sizeof(x = 2));
    }
    {
        int x = 1;
        sizeof(x = 2);
        ASSERT(1, x);
    }

    printf("OK\n");
    return 0;
}