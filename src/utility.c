#include "utility.h"

#include "generic/list.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

bool streq(char* a, char* b)
{
    assert(a && b);
    return strcmp(a, b) == 0;
}

char* strncopy(char* src, int n)
{
    assert(src);
    assert((int)strlen(src) >= n);

    char* ret = calloc(1, ALIGN(n + 1, 16));
    memcpy(ret, src, n);
    return ret;
}

bool startswithcase(char* p, char* start)
{
    for (; *start; ++p, ++start) {
        if (tolower(*start) != tolower(*p)) {
            return false;
        }
    }

    return true;
}

int path_simplify(char* inputPath, char* buf)
{
    char tmp[MAX_OSPATH];

    // 1. replace all '\\' with '/'
    strncpy(tmp, inputPath, MAX_OSPATH);
    for (char* p = tmp; *p; ++p) {
        if (*p == '\\') {
            *p = '/';
        }
    }

    // 2. resolve ./ and ../
    List* parts = list_new();

    char* l = tmp;
    char* r = tmp;
    while (r) {
        r = strchr(r + 1, '/');
        StringView part;
        part.start = l;
        part.len = (int)((r ? r : tmp + strlen(tmp)) - l);
        l = r + 1;

        list_push_back(parts, part);
    }

    List* newParts = list_new();

    for (ListNode* c = parts->front; c; c = c->next) {
        StringView* part = (StringView*)(c + 1);
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

        _list_push_back(newParts, part, sizeof(StringView));
    }

    // @TODO: prevent overflow
    int offset = 0;
    for (ListNode* c = newParts->front; c; c = c->next) {
        StringView* part = list_node_get(StringView, c);
        if (part->len == 0) {
            continue;
        }
        strncpy(buf + offset, part->start, part->len);
        offset += part->len;
        buf[offset] = '/';
        buf[offset + 1] = '\0';
        offset += 1;
    }

    int len = strlen(buf) - 1;
    buf[len] = 0; // remove last '/'

    list_clear(newParts);
    free(newParts);
    list_clear(parts);
    free(parts);

    return len;
}
