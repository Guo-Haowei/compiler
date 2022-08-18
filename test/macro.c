#include "test.h"

int return_a_plus_one()
{
#include "include1.h"
    return a + 1;
}

int main()
{
    assert(11, __LINE__);
#define MYLINE __LINE__
    assert(13, MYLINE);

#define FOO 1
    assert(1, FOO);
#define ONE_MINUS_THREE 1 - 3
    assert(-2, ONE_MINUS_THREE);
#define int64 long
    int64 num = 0;
    assert(8, sizeof(num));

#include "include1.h"
    assert(19, a);
    assert(20, return_a_plus_one());

#define ABC
    ABC;
    ABC;
    ABC;

#define ADD(a, b) ((a) + (b))

    printf("OK\n");
    return 0;
}