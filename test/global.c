#include "test.h"

int x;
static int arr[4];

static char g3 = 3;
static short g4 = 4;
int g5 = 5;
long g6 = 6;
char g7[2][3] = { "AB", "CD" };
char g8[3][3][3] = { { "A", "B", "C" }, { "D", "E", "F" }, { "G", "H", "I" } };

static int counter()
{
    static int i = 0;
    static int j = 0;
    int a = (++i) + (j += 2);
    return a * i;
}

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

    ASSERT(3, g3);
    ASSERT(4, g4);
    ASSERT(5, g5);
    ASSERT(6, g6);
    ASSERT('A', g7[0][0]);
    ASSERT('D', g7[1][1]);
    ASSERT('E', g8[1][1][0]);

    ASSERT(3, counter());
    ASSERT(12, counter());
    ASSERT(27, counter());

    printf("OK\n");
    return 0;
}