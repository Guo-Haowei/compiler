#include "test.h"

int main()
{
    {
        int x[3] = { 1, 2, 3 };
        ASSERT(1, x[0]);
        ASSERT(2, x[1]);
        ASSERT(3, x[2]);
    }
    {
        int x[2][3] = { { 1, 2, 3 }, { 4, 5, 6 } };
        ASSERT(2, x[0][1]);
        ASSERT(4, x[1][0]);
        ASSERT(6, x[1][2]);
    }
    printf("OK\n");
    return 0;
}