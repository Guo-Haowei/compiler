#include "minic.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EMPTYLINE "                                                                                "
#define UNDERLINE "^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"

enum {
    LEVEL_NOTE,
    LEVEL_WARN,
    LEVEL_ERROR,
};

#define KRESET "\033[0m"
#define KRED "\033[0;31m"
#define KYEL "\033[0;33m"
#define KBLU "\033[0;34m"

static void verror_at(int level, const char* file, const char* source, int sourceLen, int line, int col, int span, const char* fmt, va_list args)
{
    assert(file);
    assert(source);
    assert(sourceLen);
    assert(line);
    assert(col);
    assert(span);

    // fprintf(stderr, "debug verror_at(): span: %d\n", span);
    const char* color = level == LEVEL_ERROR ? KRED : KYEL;
    const char* label = "";
    switch (level) {
    case LEVEL_NOTE:
        color = KBLU;
        label = "not:";
        break;
    case LEVEL_WARN:
        color = KYEL;
        label = "warn:";
        break;
    case LEVEL_ERROR:
        color = KRED;
        label = "error:";
        break;
    default:
        assert(0);
        break;
    }

    fprintf(stderr, "%s:%d:%d: %s%s%s ", file, line, col, color, label, KRESET);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    // print line
    char const* lineStart = source;
    for (int curLine = 1; curLine < line; ++curLine) {
        lineStart = strchr(lineStart, '\n');
        if (lineStart) {
            ++lineStart;
        } else {
            lineStart = source;
        }
    }

    char const* lineEnd = lineStart;
    if ((lineEnd = strchr(lineEnd, '\n')) == NULL) {
        lineEnd = source + sourceLen;
    }

    const int lineLen = (int)(lineEnd - lineStart);
    fprintf(stderr, "%5d | %.*s\n", line, lineLen, lineStart);

    fprintf(stderr, "      |%s%.*s%.*s%s\n", color, col, EMPTYLINE, span, UNDERLINE, KRESET);
}

void error(const char* const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(-1);
}

void error_lex(const Lexer* lexer, const char* const fmt, ...)
{
    char const* file = lexer->sourceInfo->file;
    char const* source = lexer->sourceInfo->start;
    int const sourceLen = lexer->sourceInfo->len;
    int const line = lexer->line;
    int const col = lexer->col;

    va_list args;
    va_start(args, fmt);
    verror_at(LEVEL_ERROR, file, source, sourceLen, line, col, 1, fmt, args);
    va_end(args);
    exit(-1);
}

static void verror_tok_internal(int level, const Token* tok, const char* fmt, va_list args)
{
    const char* file = tok->sourceInfo->file;
    const char* source = tok->sourceInfo->start;
    const int sourceLen = tok->sourceInfo->len;
    const int line = tok->line;
    const int col = tok->col;
    int span = tok->len;
    if (tok->kind == TK_EOF) {
        span = 1;
    }

    verror_at(level, file, source, sourceLen, line, col, span, fmt, args);
}

void error_tok(const Token* tok, const char* const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    verror_tok_internal(LEVEL_ERROR, tok, fmt, args);
    va_end(args);

    const Token* macro = tok->expandedFrom;
    if (macro) {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), "in expansion of macro '%.*s'", macro->len, macro->p);
        verror_tok_internal(LEVEL_NOTE, tok->expandedFrom, buffer, NULL);
    }

    // @TODO: exit with error code
    exit(-1);
}

char const* token_kind_to_string(TokenKind eTokenKind)
{
    ASSERT_IDX(eTokenKind, TK_COUNT);

    static char const* const s_names[] = {
#define DEFINE_TOKEN(NAME) #NAME,
#include "token.inl"
#undef DEFINE_TOKEN
    };
    STATIC_ASSERT(ARRAY_COUNTER(s_names) == TK_COUNT);

    return s_names[eTokenKind];
}

char const* node_kind_to_string(NodeKind eNodeKind)
{
    ASSERT_IDX(eNodeKind, ND_COUNT);

    static char const* const s_names[] = {
#define DEFINE_NODE(NAME, BINOP, UNARYOP) #NAME,
#include "node.inl"
#undef DEFINE_NODE
    };
    STATIC_ASSERT(ARRAY_COUNTER(s_names) == ND_COUNT);

    return s_names[eNodeKind];
}

void debug_print_token(const Token* tok)
{
    if (tok->isFirstTok) {
        fprintf(stderr, "line: %d\n", tok->line);
    }

    fprintf(stderr, "  %s:%d:%d:[%s]",
        tok->sourceInfo->file,
        tok->line,
        tok->col,
        token_kind_to_string(tok->kind));
    if (tok->raw) {
        fprintf(stderr, " '%s'", tok->raw);
    } else {
        fprintf(stderr, " '%.*s'", tok->len, tok->p);
    }
    const Token* macro = tok->expandedFrom;
    if (macro) {
        fprintf(stderr, "expanded from macro (%.*s:%d:%d)", tok->len, tok->p, macro->line, macro->col);
    }
    fprintf(stderr, "\n");
}

void debug_print_tokens(List* toks)
{
    fprintf(stderr, "*** tokens ***\n");
    for (ListNode* c = toks->front; c; c = c->next) {
        Token* tok = (Token*)(c + 1);
        debug_print_token(tok);
    }
}
