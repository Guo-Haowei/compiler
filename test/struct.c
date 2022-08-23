#include "test.h"

// clang-format off
int main()
{
    {
        struct { int a; int b; } x;
        x.a = 10;
        x.b = 20;
        ASSERT(10, x.a);
        ASSERT(20, x.b);
    }
    {
        struct { char a; int b; char c; } x;
        x.a = 1;
        x.b = 2;
        x.c = 3;
        ASSERT(1, x.a);
        ASSERT(2, x.b);
        ASSERT(3, x.c);
        ASSERT(12, sizeof(x));
    }
    {
        struct { char a, b; } x[3];
        char *p = x;
        for (int i = 0; i < 4; i += 1) {
            p[i] = i;
        }
        ASSERT(0, x[0].a);
        ASSERT(1, x[0].b);
        ASSERT(2, x[1].a);
        ASSERT(3, x[1].b);
        ASSERT(6, sizeof(x));
    }
    {
        struct { char a[3]; char b[5]; } x;
        char *p = &x;
        x.a[0] = 6;
        x.b[0] = 7;
        ASSERT(6, p[0]);
        ASSERT(7, p[3]);
        ASSERT(8, sizeof(x));
    }
    {
        struct { struct { char b; } a; } x;
        x.a.b = 6;
        ASSERT(6, x.a.b);
    }
    {
        struct { int a; } x;
        ASSERT(4, sizeof(x));
    }
    {
        struct { int a, b; } x;
        struct { int a; int b; } y;
        struct { char a; char b; } z;
        struct { char a; int b; } w;
        struct { int a; char b; } u;
        ASSERT(8, sizeof(x));
        ASSERT(8, sizeof(y));
        ASSERT(2, sizeof(z));
        ASSERT(8, sizeof(w));
        ASSERT(8, sizeof(u));
    }
    {
        struct t {int a; int b;} x;
        struct t y;
        ASSERT(8, sizeof(y));
        struct tt { char a[2]; };
        struct tt z;
        ASSERT(2, sizeof(z));
        {
            struct tt { char a[4]; };
            struct tt z;
            ASSERT(4, sizeof(z));
        }
    }
    {
        struct t {int x;};
        int t=1;
        struct t y;
        y.x=2;
        ASSERT(3, t + y.x);
    }
    {
        struct t { char a; } x;
        struct t* y = &x;
        x.a = 3;
        ASSERT(3, y->a);
        y->a = 4;
        ASSERT(4, x.a);
    }
    {
        struct A { int x, y; };
        struct B { struct A* p; };
    
        struct A a;
        a.x = 1;
        a.y = 2;
        struct B b;
        b.p = &a;
        struct B* pB = &b;
        ASSERT(1, b.p->x);
        ASSERT(2, b.p->y);
        ASSERT(1, pB->p->x);
        ASSERT(2, pB->p->y);
    }
    {
        struct X {char a; long b;} x;
        struct Y {char a; short b;} y;
        ASSERT(16, sizeof(x));
        ASSERT(4, sizeof(y));
    }
    {
        struct { int a,b; } x,y;
        x.a = 3;
        y = x;
        ASSERT(3, y.a);
    }
    {
        struct t {int a,b;}; struct t x; x.a=7; struct t y; struct t *z=&y; *z=x;
        ASSERT(7, y.a);
    }
    {
        struct t {int a,b;};
        struct t x;
        x.a=7;
        struct t y, *p=&x, *q=&y;
        *q=*p;
        ASSERT(7, y.a);
    }
    {
        struct t {char a, b;} x, y; x.a=5; x.b = 100; y=x;
        ASSERT(5, y.a);
        ASSERT(100, y.b);
    }

    printf("OK\n");
    return 0;
}