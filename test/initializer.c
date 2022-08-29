#include "test.h"

int g9[] = { 0, -1, -2 };
struct {
    char a;
    int b;
} g11[2] = { { 1, 2 }, { 3, 4 } };
struct {
    int a[2];
} g12[2] = { { { 1, 2 } } };

int main()
{
    {
        int x[3] = {};
        ASSERT(0, x[0]);
        ASSERT(0, x[1]);
        ASSERT(0, x[2]);
    }
    {
        int x[2][3] = { { 1, 2 } };
        ASSERT(2, x[0][1]);
        ASSERT(0, x[1][0]);
        ASSERT(0, x[1][2]);
    }
    {
        int x[2] = { 1, 2, 4 };
        ASSERT(1, x[0]);
        ASSERT(2, x[1]);
    }
    {
        char x[5] = "abc";
        ASSERT('a', x[0]);
        ASSERT('b', x[1]);
        ASSERT('c', x[2]);
        ASSERT('\0', x[3]);
        ASSERT('\0', x[4]);
    }
    {
        char x[2][4] = { "abc", "def" };
        ASSERT('a', x[0][0]);
        ASSERT(0, x[0][3]);
        ASSERT('d', x[1][0]);
        ASSERT('f', x[1][2]);
        ASSERT(2, sizeof(x) / 4);
    }
    {
        typedef struct {
            int a, b;
            long c;
        } A;
        A x = { 1, 2, 3 };
        ASSERT(1, x.a);
        ASSERT(2, x.b);
        ASSERT(3, x.c);

        A y = { 1 };
        ASSERT(1, y.a);
        ASSERT(0, y.b);
        ASSERT(0, y.c);

        A z[2] = { { 1, 2 }, { 3, 4 } };
        ASSERT(1, z[0].a);
        ASSERT(2, z[0].b);
        ASSERT(3, z[1].a);
        ASSERT(4, z[1].b);

        A w = { 1, 2, 3, 5 };
        ASSERT(1, w.a);
        ASSERT(2, w.b);
        ASSERT(3, w.c);

        A x2 = {};
        ASSERT(0, x2.a);
        ASSERT(0, x2.b);
        ASSERT(0, x2.c);
    }
    {
        int x[] = { 1, 2, 3 };
        ASSERT(12, sizeof(x));
        ASSERT(1, x[0]);
        ASSERT(2, x[1]);
        ASSERT(3, x[2]);
    }
    {
        char x[] = "abcdef";
        ASSERT(7, sizeof(x));
        ASSERT('\0', x[6]);
    }
    {
        int x[][3] = { { 1, 2, 3 }, { 4, 5, 6 } };
        ASSERT(2, x[0][1]);
        ASSERT(4, x[1][0]);
        ASSERT(6, x[1][2]);
    }

    ASSERT(0, g9[0]);
    ASSERT(-1, g9[1]);
    ASSERT(-2, g9[2]);
    ASSERT(1, g11[0].a);
    ASSERT(2, g11[0].b);
    ASSERT(3, g11[1].a);
    ASSERT(4, g11[1].b);
    ASSERT(1, g12[0].a[0]);
    ASSERT(2, g12[0].a[1]);
    ASSERT(0, g12[1].a[0]);
    ASSERT(0, g12[1].a[1]);

    printf("OK\n");
    return 0;
}