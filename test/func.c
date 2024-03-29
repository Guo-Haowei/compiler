#include "test.h"
#include <stdio.h>
#include <string.h>

static int s_int;

void ret()
{
    ++s_int;
    return;
}

int ret8()
{
    return 8;
    return 3;
}

static int sub(int a, int b)
{
    return a - b;
}

int static fib(int x)
{
    return x <= 1 ? 1 : fib(x - 1) + fib(x - 2);
}

static int add4(int a, int b, int c, int d)
{
    return a + b + c + d;
}

static char add_char(char a, char b)
{
    return a + b;
}

static short add_short(short a, short b)
{
    return a + b;
}

static int add_int(int a, int b)
{
    return a + b;
}

static long add_long(long a, long b)
{
    return a + b;
}

static short sub_short(short a, short b, short c)
{
    return a - b - c;
}

static long sub_long(long a, long b, long c)
{
    return a - b - c;
}

int g1;

static int* g1_ptr()
{
    return &g1;
}

static char int_to_char(int x)
{
    return x;
}

static char char_fn() { return (2 << 8) + 3; }
static short short_fn() { return (2 << 16) + 5; }

static int add8(int a, int b, int c, int d, int e, int f, int g, int h)
{
    // #define P(p) printf("%s: %d\n", #p, p);
    //     P(a);
    //     P(b);
    //     P(c);
    //     P(d);
    //     P(e);
    //     P(f);
    //     P(g);
    //     P(h);
    // #undef P
    return a + b + c + d + e + f + g + h;
}

static int dummy(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j)
{
    return a + b * c - d * e - f + g * h + i + j;
}

int main()
{
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

    g1 = 3;
    ASSERT(3, *g1_ptr());
    ASSERT(5, int_to_char(261));

    ret();
    ASSERT(1, s_int);
    ASSERT(3, char_fn());
    ASSERT(5, short_fn());

    char buf[128];
    snprintf(buf, sizeof(buf), "%d%d%d%d%d%d%d%d", 1, 2, 3, 4, 5, 6, 7, 8);
    ASSERT(0, strcmp(buf, "12345678"));

    ASSERT(36, add8(1, 2, 3, 4, 5, 6, 7, 8));
    ASSERT(56, dummy(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));

    printf("OK\n");
    return 0;
}