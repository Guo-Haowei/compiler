#include "test.h"


int main()
{
    ASSERT(10, -10 + 20);
    ASSERT(10, - -10);
    ASSERT(10, - - +10);
    ASSERT(10, -(10 + 20) + 40);
    printf("OK\n");
    return 0;
}