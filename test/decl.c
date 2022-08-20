#include "test.h"

int main()
{
    {
        char a;
        short b;
        int c;
        long d;
        ASSERT(1, sizeof a);
        ASSERT(2, sizeof b);
        ASSERT(4, sizeof c);
        ASSERT(8, sizeof d);
    }
    {
        int short x1;
        ASSERT(2, sizeof x1);
        short int x2;
        ASSERT(2, sizeof x2);
    }
    {
        int long x1;
        ASSERT(8, sizeof x1);
        long int x2;
        ASSERT(8, sizeof x2);
    }
    {
        long long int x1;
        long long x2;
        int long long x3;
        long int long x4[5];
        ASSERT(8, sizeof x1);
        ASSERT(8, sizeof x2);
        ASSERT(8, sizeof x3);
        ASSERT(40, sizeof x4);
    }

    printf("OK\n");
    return 0;
}
