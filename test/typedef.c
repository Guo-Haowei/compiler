#include "test.h"

typedef int MyInt, MyInt2[4];
typedef int;

int main()
{
    {
        typedef int t;
        t x = 1;
        assert(1, x);
    }
    {
        typedef struct {
            int a;
        } t;
        t x;
        x.a = 2;
        assert(2, x.a);
    }
    {
        typedef struct {
            int a;
        } t;
        {
            typedef int t;
        }
        t x;
        x.a = 2;
        assert(2, x.a);
    }
    {
        typedef t;
        t x;
        assert(4, sizeof(x));
    }
    {
        MyInt x = 3;
        assert(3, x);
    }
    {
        MyInt2 x;
        assert(16, sizeof(x));
    }
    printf("OK\n");
    return 0;
}