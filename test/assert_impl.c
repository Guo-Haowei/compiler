#include <stdio.h>
#include <stdlib.h>

void assert_impl(int expected, int actual, const char* code, int line)
{
    if (expected == actual) {
        // printf("%s => %d\n", code, actual);
    } else {
        printf("%s => %d expected but got %d\n", code, expected, actual);
        exit(line);
    }
}

// HACK: tmp
int assert(int a, int b)
{
    assert_impl(a, b, "", 0);
    return 0;
}