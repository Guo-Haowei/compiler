#include "test.h"

int main() {
    (void)1;

    ASSERT(131585, (int)8590066177);
    ASSERT(513, (short)8590066177);
    ASSERT(1, (char)8590066177);
    ASSERT(1, (long)1);
    ASSERT(0, (long)&*(int *)0);
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
        typedef struct {
            char a[64];
        } B;
    
        B b;
        // @TODO: use string copy to set "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        int i;
        for (i = 0; i < 26; i = i + 1) {
            b.a[i] = i + 65;
        }

        A* p = (A*)&b;
        ASSERT(1145258561, p->a);
        ASSERT(1280002633, p->b);
        ASSERT(81, p->c); // @TODO: change to 'Q'
    }

    printf("OK\n");
    return 0;
}