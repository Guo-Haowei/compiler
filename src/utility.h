#ifndef __UTILITY_H__
#define __UTILITY_H__
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define ARRAY_COUNTER(arr) (sizeof(arr) / sizeof(*(arr)))
#define STATIC_ASSERT(COND) typedef char _static_assertion_[(COND) ? 1 : -1]
#define ASSERT_IDX(a, bound) assert(((int)a >= 0) && ((int)a < (int)bound))
#define UNREACHABLE() assert(0)

/// math
#define ALIGN_TO(x, a) (((x) + (a)-1) & ~((a)-1))
#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)

#ifdef ZERO_MEMORY
#undef ZERO_MEMORY
#endif
#define ZERO_MEMORY(obj) memset(&obj, 0, sizeof(obj))

typedef struct {
    const char* start;
    int len;
} StringView;

/// string
bool streq(const char* a, const char* b);
char* strncopy(const char* src, int n);
char* format(const char* fmt, ...);

/// path
#define MAX_OSPATH 512
size_t path_simplify(const char* inputPath, char* buf);

#endif // __UTILITY_H__
