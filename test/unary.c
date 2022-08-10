#include "test.h"

int main()
{
    assert(10, -10+20);
    assert(10, - -10);
    assert(10, - - +10);
    assert(10, -(10+20)+40);
    printf("OK\n");
    return 0;
}