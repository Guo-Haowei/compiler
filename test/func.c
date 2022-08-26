#include "test.h"
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

typedef struct {
    int gp_offset;
    int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} __va_elem;

typedef __va_elem va_list[1];

int add_all(int n, ...);
int sprintf(char *buf, char *fmt, ...);
int vsprintf(char *buf, char *fmt, va_list ap);

char *fmt(char *buf, char *fmt, ...) {
    va_list ap;
    *ap = *(__va_elem *)__va_area__;
    vsprintf(buf, fmt, ap);
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
    snprintf(buf, sizeof(buf), "HHHH %s", "AAAA");
    ASSERT(0, strcmp(buf, "HHHH AAAA"));

    {
        // @TODO: handle more than 4 args
        char buf[100];
        fmt(buf, "%d %s", 12, "foo");
        printf("%s\n", buf);
    }

    printf("OK\n");
    return 0;
}