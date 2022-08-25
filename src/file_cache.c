#include "minic.h"

typedef struct {
    char path[MAX_OSPATH];
    Array* tokenArray;
} FileCache;

static Array* s_filecaches;

static void fcache_init()
{
    s_filecaches = array_new(sizeof(FileCache), 4);
}

Array* fcache_get(char* path)
{
    if (!s_filecaches) {
        fcache_init();
    }

    for (int idx = 0; idx < s_filecaches->len; ++idx) {
        FileCache* fcache = array_at(FileCache, s_filecaches, idx);
        if (strcmp(fcache->path, path) == 0) {
            return fcache->tokenArray;
        }
    }
    return NULL;
}

bool fcache_add(char* path, Array* toks)
{
    if (!s_filecaches) {
        fcache_init();
    }

    for (int idx = 0; idx < s_filecaches->len; ++idx) {
        FileCache* fcache = array_at(FileCache, s_filecaches, idx);
        if (strcmp(fcache->path, path) == 0) {
            return false;
        }
    }

    FileCache fcache;
    strncpy(fcache.path, path, MAX_OSPATH);
    fcache.tokenArray = toks;
    _array_push_back(s_filecaches, &fcache);
    return true;
}
