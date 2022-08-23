#include "test.h"

int main()
{
    ASSERT(0, 0);
    ASSERT(42, 42);
    ASSERT(21, 5 + 20 - 4);
    ASSERT(41, 12 + 34 - 5);
    ASSERT(47, 5 + 6 * 7);
    ASSERT(15, 5 * (9 - 6));
    ASSERT(4, (3 + 5) / 2);
    ASSERT(10, -10 + 20);
    ASSERT(-10, -10);
    ASSERT(3, (1, 2, 3));
    ASSERT(0, 1073741824 * 100 / 100);

    {
        int i = 2;
        ASSERT(7, i += 5);
    }
    {
        int i = 2;
        i += 5;
        ASSERT(7, i);
    }
    {
        int i = 5;
        ASSERT(3, i -= 2);
    }
    {
        int i = 5;
        i -= 2;
        ASSERT(3, i);
    }
    {
        int i = 3;
        ASSERT(6, i *= 2);
    }
    {
        int i = 3;
        i *= 2;
        ASSERT(6, i);
    }
    {
        int i = 12;
        ASSERT(4, i /= 3);
    }
    {
        int i = 12;
        i /= 3;
        ASSERT(4, i);
    }
    {
        int i = 2;
        ASSERT(3, ++i);
    }
    {
        int a[3];
        a[0] = 0;
        a[1] = 1;
        a[2] = 2;
        int *p1 = a + 1;
        ASSERT(2, ++*p1);
    }

    ASSERT(5, 17%6);
    ASSERT(5, ((long)17)%6);
    {
        int i = 10;
        ASSERT(2, i %= 4);
    }

    ASSERT(0, !1);
    ASSERT(0, !2);
    ASSERT(1, !0);
    ASSERT(1, !(char)0);
    ASSERT(0, !(long)3);
    ASSERT(4, sizeof(!(char)0));
    ASSERT(4, sizeof(!(long)0));

    ASSERT(-1, ~0);
    ASSERT(0, ~-1);

    ASSERT(0, 0&1);
    ASSERT(1, 3&1);
    ASSERT(3, 7&3);
    ASSERT(10, -1&10);
    ASSERT(1, 0|1);
    ASSERT(0b10011, 0b10000|0b00011);
    ASSERT(0, 0^0);
    ASSERT(0, 0b1111^0b1111);
    ASSERT(0b110100, 0b111000^0b001100);

    {
        int i = 6;
        ASSERT(2, i &= 3);
    }
    {
        int i = 6;
        ASSERT(7, i |= 3);
    }
    {
        int i = 15;
        ASSERT(10, i ^= 5);
    }

    ASSERT(1, 0||1);
    ASSERT(1, 0||(2-2)||5);
    ASSERT(0, 0||0);
    ASSERT(0, 0||(2-2));
    ASSERT(0, 0&&1);
    ASSERT(0, (2-2)&&5);
    ASSERT(1, 1&&5);

    printf("OK\n");
    return 0;
}