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

static void verror_at(int level, char* file, char* source, int sourceLen, int line, int col, int span, char* fmt, va_list args)
{
    assert(file);
    assert(source);
    assert(sourceLen);
    assert(line);
    assert(col);
    assert(span);

    // fprintf(stderr, "debug verror_at(): span: %d\n", span);
    char* color = level == LEVEL_ERROR ? KRED : KYEL;
    char* label = "";
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
    char* lineStart = source;
    for (int curLine = 1; curLine < line; ++curLine) {
        lineStart = strchr(lineStart, '\n');
        if (lineStart) {
            ++lineStart;
        } else {
            lineStart = source;
        }
    }

    char* lineEnd = lineStart;
    if ((lineEnd = strchr(lineEnd, '\n')) == NULL) {
        lineEnd = source + sourceLen;
    }

    int lineLen = (int)(lineEnd - lineStart);
    fprintf(stderr, "%5d | ", line);
    int len1 = col - 1;
    int len2 = span;
    fprintf(stderr, "%.*s", len1, lineStart);
    fprintf(stderr, "%s%.*s%s", color, len2, lineStart + len1, KRESET);
    fprintf(stderr, "%.*s\n", lineLen - len1 - len2, lineStart + len1 + len2);

    fprintf(stderr, "      |%s%.*s%.*s%s\n", color, col, EMPTYLINE, span, UNDERLINE, KRESET);
}

void error(char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(-1);
}

void error_lex(Lexer* lexer, char* fmt, ...)
{
    char* file = lexer->sourceInfo->file;
    char* source = lexer->sourceInfo->start;
    int sourceLen = lexer->sourceInfo->len;
    int line = lexer->line;
    int col = lexer->col;

    va_list args;
    va_start(args, fmt);
    verror_at(LEVEL_ERROR, file, source, sourceLen, line, col, 1, fmt, args);
    va_end(args);
    exit(-1);
}

static void verror_tok_internal(int level, Token* tok, char* fmt, va_list args)
{
    char* file = tok->sourceInfo->file;
    char* source = tok->sourceInfo->start;
    int sourceLen = tok->sourceInfo->len;
    int line = tok->line;
    int col = tok->col;
    int span = tok->len;
    if (tok->kind == TK_EOF) {
        span = 1;
    }

    verror_at(level, file, source, sourceLen, line, col, span, fmt, args);
}

void info_tok(Token* tok, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    verror_tok_internal(LEVEL_NOTE, tok, fmt, args);
    va_end(args);
}

void error_tok(Token* tok, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    verror_tok_internal(LEVEL_ERROR, tok, fmt, args);
    va_end(args);

    Token* macro = tok->expandedFrom;
    if (macro) {
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), "in expansion of macro '%s'", macro->raw);
        verror_tok_internal(LEVEL_NOTE, tok->expandedFrom, buffer, NULL);
    }

    // @TODO: exit with error code
    exit(-1);
}

char* token_kind_to_string(TokenKind eTokenKind)
{
    ASSERT_IDX(eTokenKind, TK_COUNT);

    static char* s_names[] = {
#define DEFINE_TOKEN(NAME) #NAME,
#include "token.inl"
#undef DEFINE_TOKEN
    };

    return s_names[eTokenKind];
}

void debug_print_token(Token* tok)
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
    Token* macro = tok->expandedFrom;
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

char* format(char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int size = vsnprintf(NULL, 0, fmt, ap);
    char* buffer = calloc(1, size + 1);
    vsnprintf(buffer, size + 1, fmt, ap);
    va_end(ap);

    return buffer;
}