#include <stdio.h>
#include <stdlib.h>

void assert_impl(int expected, int actual, const char* code, int line)
{
    if (expected == actual) {
        printf("%s => %d\n", code, actual);
    } else {
        printf("%s => %d expected but got %d\n", code, expected, actual);
        printf("  on line %d\n", line);
        exit(1);
    }
}