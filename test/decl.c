#include "test.h"

int main()
{
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
    {
        int short x1;
        assert(2, sizeof x1);
        short int x2;
        assert(2, sizeof x2);
    }
    {
        int long x1;
        assert(8, sizeof x1);
        long int x2;
        assert(8, sizeof x2);
    }
    {
        long long int x1;
        long long x2;
        int long long x3;
        long int long x4[5];
        assert(8, sizeof x1);
        assert(8, sizeof x2);
        assert(8, sizeof x3);
        assert(40, sizeof x4);
    }

    printf("OK\n");
    return 0;
}

