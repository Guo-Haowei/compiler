#include "minic.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool streq(const char* a, const char* b)
{
    return strcmp(a, b) == 0;
}

char* strncopy(const char* src, int n)
{
    assert(src);
    assert(n);
    assert((int)strlen(src) >= n);

    char* ret = calloc(1, n + 1);
    memcpy(ret, src, n);
    return ret;
}

char* format(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const int size = vsnprintf(NULL, 0, fmt, ap);
    char* buffer = calloc(1, size + 1);
    vsnprintf(buffer, size + 1, fmt, ap);
    va_end(ap);

    return buffer;
}
