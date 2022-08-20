#include "test.h"

int x;
int arr[4];

int main()
{
    ASSERT(0, x);
    x = 3;
    {
        int x = 100;
        ASSERT(100, x);
    }
    ASSERT(3, x);
    arr[0] = 0;
    arr[1] = 1;
    arr[2] = 2;
    arr[3] = 3;
    ASSERT(0, arr[0]);
    ASSERT(1, arr[1]);
    ASSERT(2, arr[2]);
    ASSERT(3, arr[3]);
    ASSERT(4, sizeof(x));
    ASSERT(16, sizeof(arr));

    printf("OK\n");
    return 0;
}