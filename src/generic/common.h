#ifndef __GENERIC_COMMON_H__
#define __GENERIC_COMMON_H__

#define ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))
#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)

#define ARRAY_COUNTER(arr) (sizeof(arr) / sizeof(*(arr)))

#endif
