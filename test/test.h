#ifndef __TEST_H__
#define __TEST_H__

int printf(char* fmt, ...);
void exit(int code);

static void assert_impl(int expected, int actual, char* code, int line)
{
    if (expected == actual) {
        printf("  line %d: %s => %d\n", line, code, actual);
    } else {
        printf("  line %d: %s => %d expected but got %d\n", line, code, expected, actual);
        exit(line);
    }
}

#define ASSERT(_A_, _B_) assert_impl(_A_, _B_, #_B_, __LINE__)

#endif
