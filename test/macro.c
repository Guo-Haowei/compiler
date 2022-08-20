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

#define ADD(a, b) ((a) + (b))

    printf("OK\n");
    return 0;
}