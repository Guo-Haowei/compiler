#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <string.h>

#include "generic/common.h"

#define ASSERT_IDX(a, bound) assert(((int)a >= 0) && ((int)a < (int)bound))

/// math
#ifdef ZERO_MEMORY
#undef ZERO_MEMORY
#endif
#define ZERO_MEMORY(obj) memset(&(obj), 0, sizeof(obj))

typedef struct {
    char* start;
    int len;
} StringView;

/// string
bool streq(char* a, char* b);
char* strncopy(char* src, int n);

bool startswithcase(char* p, char* start);

/// path
#define MAX_OSPATH 512
int path_simplify(char* inputPath, char* buf);

#endif // __UTILITY_H__
