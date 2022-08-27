#include "test.h"

int main()
{
    {
        enum { ten = 1 + 2 + 3 + 4 };
        ASSERT(10, ten);
    }
    {
        int i = 0;
        switch (3) {
        case 5 - 2 + 0 * 3:
            i++;
        }
        ASSERT(1, i);
    }
    ASSERT(8, sizeof(int[1 + 1]));
    ASSERT(6, sizeof(char[8 - 2]));
    ASSERT(6, sizeof(char[2 * 3]));
    ASSERT(3, sizeof(char[12 / 4]));
    ASSERT(8, sizeof(int[12 % 10]));
    ASSERT(0b100, sizeof(char[0b110 & 0b101]));
    ASSERT(0b111, sizeof(char[0b110 | 0b101]));
    ASSERT(0b110, sizeof(char[0b111 ^ 0b001]));
    ASSERT(4, sizeof(char[1 << 2]));
    ASSERT(2, sizeof(char[4 >> 1]));
    ASSERT(2, sizeof(char[(1 == 1) + 1]));
    ASSERT(1, sizeof(char[(1 != 1) + 1]));
    ASSERT(1, sizeof(char[(1 < 1) + 1]));
    ASSERT(2, sizeof(char[(1 <= 1) + 1]));
    ASSERT(2, sizeof(char[1 ? 2 : 3]));
    ASSERT(3, sizeof(char[0 ? 2 : 3]));
    ASSERT(2, sizeof(char[!0 + 1]));
    ASSERT(1, sizeof(char[!1 + 1]));
    ASSERT(2, sizeof(char[~-3]));
    ASSERT(2, sizeof(char[(5 || 6) + 1]));
    ASSERT(1, sizeof(char[(0 || 0) + 1]));
    ASSERT(2, sizeof(char[(1 && 1) + 1]));
    ASSERT(1, sizeof(char[(1 && 0) + 1]));
    ASSERT(3, sizeof(char[(int)3]));
    ASSERT(15, sizeof(char[(char)0xffffff0f]));
    ASSERT(0x10f, sizeof(char[(short)0xffff010f]));
    ASSERT(4, sizeof(char[(int)0xfffffffffff + 5]));
    ASSERT(3, sizeof(char[(int*)16 - (int*)4]));

    {
        char x[(unsigned)1 < -1];
        ASSERT(1, sizeof(x));
    }
    {
        char x[(unsigned)1 <= -1];
        ASSERT(1, sizeof(x));
    }

    printf("OK\n");
    return 0;
}
