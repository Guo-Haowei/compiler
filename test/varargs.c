#include "test.h"
#include <stdarg.h>

int sum1(int x, ...)
{
    va_list ap;
    va_start(ap, x);
    for (;;) {
        int y = va_arg(ap, int);
        if (y == 0) {
            return x;
        }
        x += y;
    }
}

// @TODO: more args

int main()
{
    ASSERT(28, sum1(1, 8, 19, 0));
    printf("OK\n");
    return 0;
}