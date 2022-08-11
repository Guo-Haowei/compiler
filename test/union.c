#include "test.h"

int main()
{
    {
        union { int a; char b[6]; } x;
        assert(8, sizeof(x));
    }
    {
        union { int a; char b[4]; } x;
        x.a = 515;
        assert(4, sizeof(x));
        assert(3, x.b[0]);
        assert(2, x.b[1]);
        assert(0, x.b[2]);
        assert(0, x.b[3]);
    }

    printf("OK\n");
    return 0;
}