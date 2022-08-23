#include "test.h"

static int test_switch(int a) {
    int i = 0;
    switch (a % 8) {
    case 7:
        ++i;
    case 6:
        ++i;
    case 5:
        ++i;
    case 4:
        ++i;
    case 3:
        ++i;
    case 2:
        ++i;
    case 1:
        ++i;
        break;
    default:
        return 0;
    }
    return i;
}

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
        int i=0;
        for(; i<10; i++) {
            if (i == 3)
                break;
        }
        ASSERT(3, i);
    }
    {
        int i=0;
        while (1)
            if (i++ == 3)
                break;
        ASSERT(4, i);
    }
    {
        int i=0;
        for(; i < 10; i++) {
            for (;;)
                break;
            if (i == 3)
                break;
        }
        ASSERT(3, i);
    }
    {
        int i=0;
        while (1) {
            while(1)
                break;
            if (i++ == 3)
                break;
        }
        ASSERT(4, i);
    }
    {
        int i = 0, j = 0;
        for (; i < 10; i++) {
            if (i > 5)
                continue;
            j++;
        }
        ASSERT(10, i);
        ASSERT(6, j);
    }
    {
        int i = 0, j = 0;
        for(; !i; ) {
            for (; j != 10; j++)
                continue;
            break;
        }
        ASSERT(10, j);
    }
    {
        int i = 0, j = 0;
        while (i++ < 10) {
            if (i > 5)
                continue;
            j++;
        }
        ASSERT(11, i);
        ASSERT(5, j);
    }
    {
        int i = 0, j = 0;
        while (!i) {
            while (j++ != 10)
                continue;
            break;
        }
        ASSERT(11, j);
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

    {
        int i = 0;
        switch (i % 3) {
        case 0:
            i = 5;
            break;
        case 1:
            i = 6;
            break;
        case 2:
            i = 7;
            break;
        }
        ASSERT(5, i);
    }
    {
        int i = 1;
        switch (i % 3) {
        case 0:
            i = 5;
            break;
        case 1:
            i = 6;
            break;
        case 2:
            i = 7;
            break;
        }
        ASSERT(6, i);
    }
    {
        int i = 0;
        switch (-1) {
        case 0xffffffff:
            i = 3;
            break;
        }
        ASSERT(3, i);
    }

    ASSERT(0, test_switch(0));
    ASSERT(1, test_switch(1));
    ASSERT(2, test_switch(2));
    ASSERT(3, test_switch(3));
    ASSERT(4, test_switch(4));
    ASSERT(5, test_switch(5));
    ASSERT(6, test_switch(6));
    ASSERT(7, test_switch(7));

    printf("OK\n");
    return 0;
}