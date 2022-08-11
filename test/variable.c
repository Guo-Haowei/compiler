#include "test.h"

int main()
{
    {
        int _a2 = 3;
        assert(3, _a2);
    }
    {
        int foo = 3, bar = 5;
        assert(8, foo + bar);
    }
    {
        int a, b;
        a = b = 3;
        assert(6, a + b);
    }
    {
        int a = 999, b;
        b = -a;
        assert(-999, b);
    }
    {
        int i = 2, j = 3;
        (i = 5, j) = 6;
        assert(5, i);
        assert(6, j);
    }
    {
        char a;
        short b;
        int c;
        long d;
        assert(1, sizeof a);
        assert(2, sizeof b);
        assert(4, sizeof c);
        assert(8, sizeof d);
    }

    printf("OK\n");
    return 0;
}
