#include "test.h"

int main()
{
    {
        int _a2 = 3;
        ASSERT(3, _a2);
    }
    {
        int foo = 3, bar = 5;
        ASSERT(8, foo + bar);
    }
    {
        int a, b;
        a = b = 3;
        ASSERT(6, a + b);
    }
    {
        int a = 999, b;
        b = -a;
        ASSERT(-999, b);
    }
    {
        int i = 2, j = 3;
        (i = 5, j) = 6;
        ASSERT(5, i);
        ASSERT(6, j);
    }
    {
        void* x;
    }
    printf("OK\n");
    return 0;
}
