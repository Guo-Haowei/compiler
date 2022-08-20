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
    // {
    //     struct { char a; int b; char c; } x;
    //     x.a = 1;
    //     x.b = 2;
    //     x.c = 3;
    //     assert(1, x.a);
    //     assert(2, x.b);
    //     assert(3, x.c);
    //     assert(12, sizeof(x));
    // }
    // {
    //     struct { char a, b; } x[3];
    //     char *p = x;
    //     int i;
    //     for (i = 0; i < 4; i = i + 1) p[i] = i;
    //     assert(0, x[0].a);
    //     assert(1, x[0].b);
    //     assert(2, x[1].a);
    //     assert(3, x[1].b);
    //     assert(6, sizeof(x));
    // }
    // {
    //     struct { char a[3]; char b[5]; } x;
    //     char *p = &x;
    //     x.a[0] = 6;
    //     x.b[0] = 7;
    //     assert(6, p[0]);
    //     assert(7, p[3]);
    //     assert(8, sizeof(x));
    // }
    // {
    //     struct { struct { char b; } a; } x;
    //     x.a.b = 6;
    //     assert(6, x.a.b);
    // }
    // {
    //     struct { int a; } x;
    //     assert(4, sizeof(x));
    // }
    // {
    //     struct { int a, b; } x;
    //     struct { int a; int b; } y;
    //     struct { char a; char b; } z;
    //     struct { char a; int b; } w;
    //     struct { int a; char b; } u;
    //     assert(8, sizeof(x));
    //     assert(8, sizeof(y));
    //     assert(2, sizeof(z));
    //     assert(8, sizeof(w));
    //     assert(8, sizeof(u));
    // }
    // {
    //     struct t {int a; int b;} x;
    //     struct t y;
    //     assert(8, sizeof(y));
    //     struct tt { char a[2]; };
    //     struct tt z;
    //     assert(2, sizeof(z));
    //     {
    //         struct tt { char a[4]; };
    //         struct tt z;
    //         assert(4, sizeof(z));
    //     }
    // }
    // {
    //     struct t {int x;};
    //     int t=1;
    //     struct t y;
    //     y.x=2;
    //     assert(3, t + y.x);
    // }
    // {
    //     struct t { char a; } x;
    //     struct t* y = &x;
    //     x.a = 3;
    //     assert(3, y->a);
    //     y->a = 4;
    //     assert(4, x.a);
    // }
    // {
    //     struct A { int x, y; };
    //     struct B { struct A* p; };
    
    //     struct A a;
    //     a.x = 1;
    //     a.y = 2;
    //     struct B b;
    //     b.p = &a;
    //     struct B* pB = &b;
    //     assert(1, b.p->x);
    //     assert(2, b.p->y);
    //     assert(1, pB->p->x);
    //     assert(2, pB->p->y);
    // }
    // {
    //     struct X {char a; long b;} x;
    //     struct Y {char a; short b;} y;
    //     assert(16, sizeof(x));
    //     assert(4, sizeof(y));
    // }


    printf("OK\n");
    return 0;
}