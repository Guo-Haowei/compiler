#include "test.h"
#include <string.h>

int main()
{
    (void)1;

    ASSERT(131585, (int)8590066177);
    ASSERT(513, (short)8590066177);
    ASSERT(1, (char)8590066177);
    ASSERT(1, (long)1);
    ASSERT(0, (long)&*(int*)0);
    {
        int x = 512;
        *(char*)&x = 1;
        ASSERT(513, x);
    }
    {
        int x = 5;
        long y = (long)&x;
        ASSERT(5, *(int*)y);
    }
    {
        typedef struct {
            int a;
            long b;
            char c;
        } A;
        struct {
            char a[64];
        } b;

        strcpy(b.a, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

        A* p = (A*)&b;
        ASSERT(1145258561, p->a);
        ASSERT(1280002633, p->b);
        ASSERT('Q', p->c);
    }

    printf("OK\n");
    return 0;
}