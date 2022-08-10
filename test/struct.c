#include "test.h"

int main()
{
    {
        struct { int a; int b; } x;
        x.a = 10;
        x.b = 20;
        assert(10, x.a);
        assert(20, x.b);
    }
    {
        struct { char a; int b; char c; } x;
        x.a = 1;
        x.b = 2;
        x.c = 3;
        assert(1, x.a);
        assert(2, x.b);
        assert(3, x.c);
        assert(24, sizeof(x));
    }
    {
        struct { char a, b; } x[3];
        char *p = x;
        int i;
        for (i = 0; i < 4; i = i + 1) p[i] = i;
        assert(0, x[0].a);
        assert(1, x[0].b);
        assert(2, x[1].a);
        assert(3, x[1].b);
        assert(6, sizeof(x));
    }
    {
        struct { char a[3]; char b[5]; } x;
        char *p = &x;
        x.a[0] = 6;
        x.b[0] = 7;
        assert(6, p[0]);
        assert(7, p[3]);
        assert(8, sizeof(x));
    }
    {
        struct { struct { char b; } a; } x;
        x.a.b = 6;
        assert(6, x.a.b);
    }
    {
        struct { int a; } x;
        assert(8, sizeof(x));
    }
    {
        struct { int a, b; } x;
        struct { int a; int b; } y;
        struct { char a; char b; } z;
        struct { char a; int b; } w;
        struct { int a; char b; } u;
        assert(16, sizeof(x));
        assert(16, sizeof(y));
        assert(2, sizeof(z));
        assert(16, sizeof(w));
        assert(16, sizeof(u));
    }

    printf("OK\n");
    return 0;
}