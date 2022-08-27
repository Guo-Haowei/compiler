#include "test.h"

int main()
{
    // clang-format off
    { int x; ASSERT(4, sizeof(x)); }
    { int x; ASSERT(4, sizeof x); }
    { int* x; ASSERT(8, sizeof(x)); }
    { int x[4]; ASSERT(16, sizeof(x)); }
    { int x[3][4]; ASSERT(48, sizeof(x)); }
    { int x[3][4]; ASSERT(16, sizeof(*x)); }
    { int x[3][4]; ASSERT(4, sizeof(**x)); }
    { int x[3][4]; ASSERT(5, sizeof(**x) + 1); }
    { int x[3][4]; ASSERT(5, sizeof **x + 1); }
    { int x[3][4]; ASSERT(4, sizeof(**x + 1)); }
    { int x = 1; ASSERT(4, sizeof(x = 2)); }
    { int x = 1; sizeof(x = 2); ASSERT(1, x); }
    // clang-format on

    ASSERT(1, sizeof(char));
    ASSERT(2, sizeof(short));
    ASSERT(2, sizeof(short int));
    ASSERT(2, sizeof(int short));
    ASSERT(4, sizeof(int));
    ASSERT(8, sizeof(long));
    ASSERT(8, sizeof(long int));
    ASSERT(8, sizeof(long int));
    ASSERT(8, sizeof(char*));
    ASSERT(8, sizeof(int*));
    ASSERT(8, sizeof(long*));
    ASSERT(8, sizeof(int**));
    // ASSERT(8, sizeof(int (*)[4]));
    ASSERT(32, sizeof(int* [4]));
    ASSERT(16, sizeof(int[4]));
    ASSERT(48, sizeof(int[3][4]));
    ASSERT(8, sizeof(struct {int a; int b; }));

    ASSERT(8, sizeof(-10 + (long)5));
    ASSERT(8, sizeof(-10 - (long)5));
    ASSERT(8, sizeof(-10 * (long)5));
    ASSERT(8, sizeof(-10 / (long)5));
    ASSERT(8, sizeof((long)-10 + 5));
    ASSERT(8, sizeof((long)-10 - 5));
    ASSERT(8, sizeof((long)-10 * 5));
    ASSERT(8, sizeof((long)-10 / 5));

    {
        int i = 1;
        sizeof(i *= 8);
        ASSERT(1, i);
        ASSERT(4, sizeof(i *= 8));
    }
    {
        char i = 1;
        sizeof(++i);
        ASSERT(1, i);
        ASSERT(1, sizeof(++i));
        ASSERT(1, sizeof(i++));
        ASSERT(1, sizeof(--i));
        ASSERT(1, sizeof(i--));
    }

    ASSERT(1, sizeof(char));
    ASSERT(1, sizeof(signed char));
    ASSERT(1, sizeof(signed char signed));
    ASSERT(1, sizeof(unsigned char));
    ASSERT(1, sizeof(unsigned char unsigned));
    ASSERT(2, sizeof(short));
    ASSERT(2, sizeof(int short));
    ASSERT(2, sizeof(short int));
    ASSERT(2, sizeof(signed short));
    ASSERT(2, sizeof(int short signed));
    ASSERT(2, sizeof(unsigned short));
    ASSERT(2, sizeof(int short unsigned));
    ASSERT(4, sizeof(int));
    ASSERT(4, sizeof(signed int));
    ASSERT(4, sizeof(signed));
    ASSERT(4, sizeof(signed signed));
    ASSERT(4, sizeof(unsigned int));
    ASSERT(4, sizeof(unsigned));
    ASSERT(4, sizeof(unsigned unsigned));
    ASSERT(8, sizeof(long));
    ASSERT(8, sizeof(signed long));
    ASSERT(8, sizeof(signed long int));
    ASSERT(8, sizeof(unsigned long));
    ASSERT(8, sizeof(unsigned long int));
    ASSERT(8, sizeof(long long));
    ASSERT(8, sizeof(signed long long));
    ASSERT(8, sizeof(signed long long int));
    ASSERT(8, sizeof(unsigned long long));
    ASSERT(8, sizeof(unsigned long long int));
    ASSERT(1, sizeof((char)1));
    ASSERT(2, sizeof((short)1));
    ASSERT(4, sizeof((int)1));
    ASSERT(8, sizeof((long)1));
    ASSERT(4, sizeof((char)1 + (char)1));
    ASSERT(4, sizeof((short)1 + (short)1));
    ASSERT(4, sizeof(1 ? 2 : 3));
    ASSERT(4, sizeof(1 ? (short)2 : (char)3));
    ASSERT(8, sizeof(1 ? (long)2 : (char)3));

    ASSERT(1, sizeof(char) << 31 >> 31);
    ASSERT(1, sizeof(char) << 63 >> 63);

    printf("OK\n");
    return 0;
}
