#include "test.h"

int main()
{
    {
        int abc123 = 0;
        int c;
        if (abc123)
            c = 2;
        else
            c = 3;
        ASSERT(3, c);
    }
    {
        int abc123 = 0;
        int c;
        if (abc123 + 100)
            c = 2;
        else
            c = 3;
        ASSERT(2, c);
    }
    {
        int l = -1, r = 1;
        int c;
        if (l > r)
            c = 20;
        else
            c = 30;
        ASSERT(30, c);
    }
    printf("OK\n");
    return 0;
}