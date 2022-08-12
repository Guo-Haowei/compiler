#include "minic.h"

typedef struct {
    char absPath[MAX_OSPATH];
    Array* tokenArray;
} FileCache;

static Array s_filecaches;

Array* fcache_get(const char* absPath)
{
    for (int idx = 0; idx < s_filecaches.len; ++idx) {
        FileCache* fcache = array_at(FileCache, &s_filecaches, idx);
        if (strcmp(fcache->absPath, absPath) == 0) {
            return fcache->tokenArray;
        }
    }
    return NULL;
}

bool fcache_add(const char* absPath, Array* toks)
{
    for (int idx = 0; idx < s_filecaches.len; ++idx) {
        FileCache* fcache = array_at(FileCache, &s_filecaches, idx);
        if (strcmp(fcache->absPath, absPath) == 0) {
            return false;
        }
    }

    FileCache fcache;
    strncpy(fcache.absPath, absPath, MAX_OSPATH);
    fcache.tokenArray = toks;
    return true;
}
