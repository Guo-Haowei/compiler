#include "generic/list.h"
#include "minic.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

bool streq(const char* a, const char* b)
{
    return strcmp(a, b) == 0;
}

char* strncopy(const char* src, int n)
{
    assert(src);
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

size_t path_simplify(const char* inputPath, char* buf)
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
    struct List* parts = list_new();

    char* l = tmp;
    char* r = tmp;
    while (r) {
        r = strchr(r + 1, '/');
        StringView part;
        part.start = l;
        part.len = (int)(r - l);
        l = r + 1;

        list_push_back(parts, part);
    }

    struct List* newParts = list_new();

    for (struct ListNode* c = parts->front; c; c = c->next) {
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
    for (struct ListNode* c = newParts->front; c; c = c->next) {
        StringView* part = list_node_get(StringView, c);
        if (part->len == 0) {
            continue;
        }
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

Token* tr_peek_n(TokenReader* reader, int n)
{
    ListNode* c = reader->cursor;
    if (n >= 0) {
        for (int i = 0; i < n; ++i) {
            if (!c) {
                return NULL;
            }
            c = c->next;
        }
    } else {
        for (int i = 0; i < -n; ++i) {
            if (!c) {
                return NULL;
            }
            c = c->prev;
        }
    }
    return c ? (Token*)(c + 1) : NULL;
}

Token* tr_peek(TokenReader* reader)
{
    return tr_peek_n(reader, 0);
}

Token* tr_read(TokenReader* reader)
{
    Token* ret = tr_peek(reader);
    assert(ret);
    reader->cursor = reader->cursor->next;
    return ret;
}

bool tr_equal(TokenReader* reader, const char* symbol)
{
    return is_token_equal(tr_peek(reader), symbol);
}

bool tr_consume(TokenReader* reader, const char* symbol)
{
    if (tr_equal(reader, symbol)) {
        assert(reader->cursor);
        reader->cursor = reader->cursor->next;
        return true;
    }

    return false;
}

void tr_expect(TokenReader* reader, const char* symbol)
{
    Token* token = tr_peek(reader);
    if (is_token_equal(token, symbol)) {
        reader->cursor = reader->cursor->next;
        return;
    }

    error_tok(token, "expected '%s', got '%s'", symbol, token->raw);
}