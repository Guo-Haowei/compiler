#include "test.h"

int main()
{
    {
        int i=0, j=0;
        for (i=0; i<=10; i=i+1)
            j=i+j;
        assert(55, j);
    }
    {
        int i = 0;
        while (i < 10) { i = i + 1; }
        assert(10, i);
    }
    printf("OK\n");
    return 0;
}