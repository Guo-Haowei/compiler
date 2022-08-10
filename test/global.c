#include "test.h"

int x;
int arr[4];

int main()
{
    assert(0, x);
    x = 3;
    {
        int x = 100;
        assert(100, x);
    }
    assert(3, x);
    arr[0] = 0;
    arr[1] = 1;
    arr[2] = 2;
    arr[3] = 3;
    assert(0, arr[0]);
    assert(1, arr[1]);
    assert(2, arr[2]);
    assert(3, arr[3]);
    assert(8, sizeof(x));
    assert(32, sizeof(arr));

    printf("OK\n");
    return 0;
}