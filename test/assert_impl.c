#include <stdio.h>
#include <stdlib.h>

void assert_impl(int expected, int actual, const char* code, int line)
{
    if (expected == actual) {
        printf("  line %d: %s => %d\n", line, code, actual);
    } else {
        printf("  line %d: %s => %d expected but got %d\n", line, code, expected, actual);
        exit(line);
    }
}
