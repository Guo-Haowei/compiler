#include "test.h"

int main() {
    ASSERT((long)-5, -10 + (long)5);
    ASSERT((long)-15, -10 - (long)5);
    ASSERT((long)-50, -10 * (long)5);
    ASSERT((long)-2, -10 / (long)5);
    ASSERT(1, -2 < (long)-1);
    ASSERT(1, -2 <= (long)-1);
    ASSERT(0, -2 > (long)-1);
    ASSERT(0, -2 >= (long)-1);
    ASSERT(1, (long)-2 < -1);
    ASSERT(1, (long)-2 <= -1);
    ASSERT(0, (long)-2 > -1);
    ASSERT(0, (long)-2 >= -1);
    ASSERT(0, 2147483647 + 2147483647 + 2);

    {
        long x = -1;
        ASSERT((long)-1, x);
    }
    {
        char x[3]; x[0]=0; x[1]=1; x[2]=2; char *y=x+1;
        ASSERT(1, y[0]);
    }
    {
        char x[3]; x[0]=0; x[1]=1; x[2]=2; char *y=x+1;
        ASSERT(0, y[-1]);
    }
    {
        struct t {char a;} x, y; x.a=5; y=x;
        ASSERT(5, y.a);
    }

    printf("OK\n");
    return 0;
}