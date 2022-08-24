#ifndef __ASSERT_H__
#define __ASSERT_H__
#include <stdio.h>
#include <stdlib.h>

#define assert(expr) (!!(expr) || _assert(#expr, __LINE__, __FILE__))

static int _assert(char* msg, int line, char* file)
{
    printf("Assertion failed: %s, file %s, line %d\n", msg, file, line);
    exit(1);
    return 0;
}

#endif