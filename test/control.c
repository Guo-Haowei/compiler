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

    ASSERT(1, 0||1);
    ASSERT(1, 0||(2-2)||5);
    ASSERT(0, 0||0);
    ASSERT(0, 0||(2-2));
    ASSERT(0, 0&&1);
    ASSERT(0, (2-2)&&5);
    ASSERT(1, 1&&5);

    {
        int i = 0;
        goto label1;
label1:
        i++;
label2:
        i++;
label3:
        i++;
        ASSERT(3, i);
    }
    {
        int i = 0;
        goto label5;
label4:
        i++;
label5:
        i++;
label6:
        i++;
        ASSERT(2, i);
    }
    {
        typedef int foo;
        goto foo;
            ASSERT(0, 1);
        foo:
            (void)0;
    }
    printf("OK\n");
    return 0;
}