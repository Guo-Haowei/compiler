#include "test.h"

int ret8() { return 8; }
int sub(int a, int b) { return a - b; }
int fib(int x) { if (x <= 1) return 1; return fib(x-1) + fib(x-2); }
int add4(int a, int b, int c, int d) { return a + b + c + d; }

int main()
{
    assert(77, abs(-77));
    assert(7, abs(7));
    assert(8, ret8());
    assert(13, sub(16, 3));
    assert(55, fib(9));
    assert(30, add4(1, 2, add4(1, 2, 3, 4), add4(1, 2, add4(1, 2, 3, 4), 4)));
    printf("OK\n");
    return 0;
}