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
    {
        int x[3] = {};
        ASSERT(0, x[0]);
        ASSERT(0, x[1]);
        ASSERT(0, x[2]);
    }
    {
        int x[2][3] = { { 1, 2 } };
        ASSERT(2, x[0][1]);
        ASSERT(0, x[1][0]);
        ASSERT(0, x[1][2]);
    }
    {
        int x[2] = { 1, 2, 3, 4 };
        ASSERT(1, x[0]);
        ASSERT(2, x[1]);
    }
    {
        char x[5] = "abc";
        ASSERT('a', x[0]);
        ASSERT('b', x[1]);
        ASSERT('c', x[2]);
        ASSERT('\0', x[3]);
        ASSERT('\0', x[4]);
    }
    {
        char x[2][4] = { "abc", "def" };
        ASSERT('a', x[0][0]);
        ASSERT(0, x[0][3]);
        ASSERT('d', x[1][0]);
        ASSERT('f', x[1][2]);
    }

    printf("OK\n");
    return 0;
}