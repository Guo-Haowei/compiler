#include "test.h"

int ret8() { return 8; }
int sub(int a, int b) { return a - b; }
int fib(int x)
{
    if (x <= 1)
        return 1;
    return fib(x - 1) + fib(x - 2);
}
int add4(int a, int b, int c, int d) { return a + b + c + d; }
char add_char(char a, char b) { return a + b; }
char add_short(short a, short b) { return a + b; }
char add_int(int a, int b) { return a + b; }
char add_long(long a, long b) { return a + b; }
int sub_short(short a, short b, short c) { return a - b - c; }
int sub_long(long a, long b, long c) { return a - b - c; }

int main()
{
    ASSERT(77, abs(-77));
    ASSERT(7, abs(7));
    ASSERT(8, ret8());
    ASSERT(13, sub(16, 3));
    ASSERT(55, fib(9));
    ASSERT(30, add4(1, 2, add4(1, 2, 3, 4), add4(1, 2, add4(1, 2, 3, 4), 4)));
    ASSERT(1, sub_short(7, 3, 3));
    ASSERT(1, sub_long(7, 3, 3));

    ASSERT(0, add_char(1, 256 - 1));
    ASSERT(256, add_short(1, 256 - 1));
    ASSERT(0, add_short(1, 65536 - 1));
    ASSERT(65536, add_int(1, 65536 - 1));
    ASSERT(0, add_int(1, 4294967296 - 1));
    ASSERT(4294967296, add_long(1, 4294967296 - 1));

    printf("OK\n");
    return 0;
}