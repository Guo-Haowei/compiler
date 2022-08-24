#include "test.h"

int return_a_plus_one()
{
#include "include1.h"
    return a + 1;
}

int main()
{
    ASSERT(11, __LINE__);
#define MYLINE __LINE__
    ASSERT(13, MYLINE);

#define FOO 1
    ASSERT(1, FOO);
#define ONE_MINUS_THREE 1 - 3
    ASSERT(-2, ONE_MINUS_THREE);
#define int64 long
    int64 num = 0;
    ASSERT(8, sizeof(num));

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

#define ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))
    ASSERT(16, ALIGN(1, 16));
    ASSERT(16, ALIGN(8, 16));
    ASSERT(16, ALIGN(15, 16));
    ASSERT(16, ALIGN(16, 16));
    ASSERT(32, ALIGN(18, 16));
    ASSERT(32, ALIGN(31, 16));
    ASSERT(32, ALIGN(32, 16));

    printf("OK\n");
    return 0;
}