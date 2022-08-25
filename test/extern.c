#include "extern.h"
#include "test.h"

int main()
{
#define VALUE 100
    g_int = VALUE;
    ASSERT(VALUE, g_int);

    g_struct.a = g_struct.b = 1;
    ASSERT(1, g_struct.a);
    ASSERT(1, g_struct.b);

    printf("OK\n");
    return 0;
}