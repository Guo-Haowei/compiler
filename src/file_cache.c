#include "minic.h"

#ifdef _MSC_VER
#include <Windows.h>
#define getcwd(BUF, BUFSIZE) GetCurrentDirectoryA(BUFSIZE, BUF)
#else
#include <unistd.h>
#endif

typedef struct {
    char absPath[MAX_OSPATH];
    Array* tokenArray;
} FileCache;

typedef struct {
    const char* start;
    int len;
} PathPart;

static Array s_filecaches;

static char* get_cwd_path()
{
    static char s_cwd[MAX_OSPATH];
    if (s_cwd[0] == 0) {
        if (!getcwd(s_cwd, MAX_OSPATH)) {
            assert(0);
            return s_cwd;
        }
        assert(s_cwd[0]);
    }

    return s_cwd;
}

size_t fcache_resolve_path(const char* basePath, const char* relPath, char* buf)
{
    char tmp[MAX_OSPATH];

    // 1. append cwd
    snprintf(tmp, MAX_OSPATH, "%s/%s", basePath, relPath);

    // 2. replace all '\\' with '/'
    for (char* p = tmp; *p; ++p) {
        if (*p == '\\') {
            *p = '/';
        }
    }

    // 3. resolve ./ and ../
    List* parts = list_new();

    char* l = tmp;
    char* r = tmp;
    while (r) {
        r = strchr(r + 1, '/');
        PathPart part;
        part.start = l;
        part.len = (int)(r - l);
        l = r + 1;

        list_push_back(parts, part);
    }

    List* newParts = list_new();

    for (ListNode* c = parts->front; c; c = c->next) {
        PathPart* part = (PathPart*)(c + 1);
        if (part->len == 1 && part->start[0] == '.') {
            continue;
        }

        if (part->len == 2 && part->start[0] == '.' && part->start[1] == '.') {
            // if there's something to pop, pop
            if (!list_is_empty(newParts)) {
                list_pop_back(newParts);
                continue;
            }
        }

        _list_push_back(newParts, part, sizeof(PathPart));
    }

    // @TODO: prevent overflow
    int offset = 0;
    for (ListNode* c = newParts->front; c; c = c->next) {
        PathPart* part = (PathPart*)(c + 1);
        sprintf(buf + offset, "%.*s/", part->len, part->start);
        offset += (1 + part->len);
    }

    size_t len = strlen(buf) - 1;
    buf[len] = 0; // remove last '/'

    list_clear(newParts);
    free(newParts);
    list_clear(parts);
    free(parts);

    return len;
}

size_t fcache_abs_path(const char* relPath, char* buf)
{
    return fcache_resolve_path(get_cwd_path(), relPath, buf);
}

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

_Bool fcache_add(const char* absPath, Array* toks)
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
