#include "test.h"

int main()
{
    {
        int j = 0;
        for (int i = 0; i <= 10; i = i + 1)
            j = i + j;
        ASSERT(55, j);
    }
    {
        int i = 0;
        while (i < 10) {
            i = i + 1;
        }
        ASSERT(10, i);
    }
    {
        int counter = 0;
        for (int i = 10, j = 0; i > j; i = i - 1, j = j + 1, counter = counter + 1);
        ASSERT(5, counter);
    }
    {
        int i = 1;
        for (int i = 0; i < 10; i = i + 1) {
        }
        ASSERT(1, i);
    }
    printf("OK\n");
    return 0;
}