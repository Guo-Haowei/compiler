#include "test.h"
#include <string.h>

int return_a_plus_one()
{
#include "include1.h"
    return a + 1;
}

int main()
{
    ASSERT(12, __LINE__);
#define MYLINE __LINE__
    ASSERT(14, MYLINE);

#define FOO 1
    ASSERT(1, FOO);
#define ONE_MINUS_THREE 1 - 3
    ASSERT(-2, ONE_MINUS_THREE);
#define int64 long
    int64 num = 0;
    ASSERT(8, sizeof(num));

    {
        char* f = __FILE__;
        char* p = strrchr(f, '/');
        ASSERT(0, strcmp(p, "/preproc.c"));
    }

#include "include1.h"
    ASSERT(19, a);
    ASSERT(20, return_a_plus_one());

#define ABC
    ABC;
    ABC;
    ABC;

#ifdef FGHLJHGFGHJK
    ASSERT(0, 1);
#endif

#ifdef ABC
#ifndef int64
    ASSERT(0, 1);
#endif
#else
    ASSERT(0, 1);
#endif

#ifndef ABC
#ifdef FOO
    aaaaaaaaaaaaaaaaa;
#endif
#endif

#undef ABC
#ifdef ABC
    printf("You shouldn't see this\n");
#ifndef int64
    ASSERT(0, 1);
#endif
#endif

#define ABC ("ABC")
    ASSERT('A', ABC[0]);

#define PRINTLN(...)         \
    {                        \
        printf(__VA_ARGS__); \
        printf("\n");        \
    }                        \
    (void)0

#define CONCAT(a, b, c) __var_##a##_##b##_##c##__
    int CONCAT(dummy, hello, world) = 10;
    ASSERT(10, __var_dummy_hello_world__);

#if 0 + 1 + 2
    ASSERT(0, 0);
#endif

#if 100 - 4 * 25
#error 100 - 4 * 25
#else
#if !(10 > 20)
    ASSERT(0, 0);
#else
#error !(10 > 20)
#endif
#endif

#warning this is a warning

#if !defined(CMDPREPROC)
#error CMDPREPROC not defined
#endif

#if defined(TWO)
    ASSERT(2, TWO);
#else
    ASSERT(0, 1);
#endif

#if defined(AB) || (1 + 1 == 2)
#define ABCDE
#if defined(ABCDE) && 7
    printf("%c", 'O');
#endif
#endif

#if !defined(ABCD)
    PRINTLN("%c", 'K');
#endif
    return 0;
}