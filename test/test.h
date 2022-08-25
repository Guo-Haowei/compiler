#include <stdio.h>
#include <stdlib.h>

// int assert_impl(int a, int b, char* msg, int ln);
#define ASSERT(_A_, _B_) assert_impl(_A_, _B_, #_B_, __LINE__)

static void assert_impl(int expected, int actual, char* code, int line)
{
    if (expected == actual) {
        printf("  line %d: %s => %d\n", line, code, actual);
    } else {
        // HACK: only support 4 args for now
        printf("  line %d: ", line);
        printf("%s => %d expected but got %d\n", code, expected, actual);
        exit(line);
    }
}