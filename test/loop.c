#include "test.h"

int main()
{
    {
        int j = 0;
        for (int i = 0; i <= 10; ++i)
            j = i + j;
        ASSERT(55, j);
    }
    {
        int i = 0;
        while (i < 10) {
            i += 2;
        }
        ASSERT(10, i);
    }
    {
        int counter = 0;
        for (int i = 10, j = 0; i > j; i--, j += 1, counter++);
        ASSERT(5, counter);
    }
    {
        int i = 1;
        for (int i = 0; i < 10; i++) {
        }
        ASSERT(1, i);
    }
    {
        int a[3];
        a[0]=0; a[1]=1; a[2]=2;
        int *p=a+1; (*p++)--;
        ASSERT(0, a[1]);
        ASSERT(p, a + 2);
    }
    {
        int a[3];
        a[0]=0; a[1]=1; a[2]=2;
        int *p=a+1; (*(p--))--;
        ASSERT(0, a[1]);
        ASSERT(p, a);
    }
    // ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p)--; a[2]; }));
    // ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p)--; p++; *p; }));
    // ASSERT(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[0]; }));
    // ASSERT(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[1]; }));
    // ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[2]; }));
    // ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; *p; }));

    printf("OK\n");
    return 0;
}