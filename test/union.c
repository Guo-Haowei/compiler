#include "test.h"

int main()
{
    {
        union {
            int a;
            char b[6];
        } x;
        ASSERT(8, sizeof(x));
    }
    {
        union {
            int a;
            char b[4];
        } x;
        x.a = 515;
        ASSERT(4, sizeof(x));
        ASSERT(3, x.b[0]);
        ASSERT(2, x.b[1]);
        ASSERT(0, x.b[2]);
        ASSERT(0, x.b[3]);
    }

    printf("OK\n");
    return 0;
}