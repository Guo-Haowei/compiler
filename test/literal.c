#include "test.h"

int main()
{
    ASSERT(97, 'a');
    ASSERT(8, '\b');
    ASSERT(10, '\n');
    ASSERT(92, '\\');
    ASSERT(0, '\0');
    ASSERT(-128, '\x80');

    printf("OK\n");
    return 0;
}