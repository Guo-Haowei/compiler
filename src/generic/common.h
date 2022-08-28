#ifndef __GENERIC_COMMON_H__
#define __GENERIC_COMMON_H__

#define ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))
#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)

#define ARRAY_COUNTER(ARR) (sizeof(ARR) / sizeof(*(ARR)))
#define STATIC_ASSERT(COND)      \
    {                            \
        int _a[(COND) ? 1 : -1]; \
        ((void)_a[0]);           \
    }                            \
    ((void)0)

// define bool as int
typedef int bool;
#define true (1)
#define false (0)

#endif
