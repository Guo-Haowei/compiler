#include "minic.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void verror_at(char const* file, char const* source, int sourceLen, int line, int col, int span, char const* const fmt, va_list args)
{
    printf("%s:%d:%d: error: ", file, line, col);
    vprintf(fmt, args);
    printf("\n");

    // print line
    char const* lineStart = source;
    for (int curLine = 1; curLine < line; ++curLine) {
        lineStart = strchr(lineStart, '\n');
        assert(lineStart);
        ++lineStart;
    }

    char const* lineEnd = lineStart;
    if ((lineEnd = strchr(lineEnd, '\n')) == nullptr) {
        lineEnd = source + sourceLen;
    }

    int const lineLen = lineEnd - lineStart;
    printf("%5d | %.*s\n", line, lineLen, lineStart);

    // print underscore
    printf("      |%*c%*c\n", col, ' ', span, '^');

    exit(-1);
}

void error_at_lexer(Lexer const* lexer, char const* const fmt, ...)
{
    char const* file = lexer->sourceInfo->file;
    char const* source = lexer->sourceInfo->start;
    int const sourceLen = lexer->sourceInfo->len;
    int const line = lexer->line;
    int const col = lexer->col;

    va_list args;
    va_start(args, fmt);
    verror_at(file, source, sourceLen, line, col, 1, fmt, args);
    va_end(args);
}

void error_at_token(Token const* tok, char const* const fmt, ...)
{
    char const* file = tok->sourceInfo->file;
    char const* source = tok->sourceInfo->start;
    int const sourceLen = tok->sourceInfo->len;
    int const line = tok->line;
    int const col = tok->col;
    int const span = tok->len;

    va_list args;
    va_start(args, fmt);
    verror_at(file, source, sourceLen, line, col, span, fmt, args);
    va_end(args);
}

char const* token_kind_to_string(TokenKind eTokenKind)
{
    switch (eTokenKind) {
    case TK_PUNCT:
        return "TK_PUNCT";
    case TK_NUM:
        return "TK_NUM";
    case TK_EOF:
        return "TK_EOF";
    }

    assert(0 && "not reachable");
    return "";
}

char const* node_kind_to_string(NodeKind eNodeKind)
{
    switch (eNodeKind) {
    case ND_INVALID:
        return "ND_INVALID";
    case ND_ADD:
        return "ND_ADD";
    case ND_SUB:
        return "ND_SUB";
    case ND_MUL:
        return "ND_MUL";
    case ND_DIV:
        return "ND_DIV";
    case ND_REM:
        return "ND_REM";
    case ND_NEG:
        return "ND_NEG";
    case ND_NUM:
        return "ND_NUM";
    }

    assert(0 && "not reachable");
    return "";
}

static char const* node_kind_to_symbol(NodeKind eNodeKind)
{
    switch (eNodeKind) {
    case ND_INVALID:
        return "<error>";
    case ND_ADD:
        return "+";
    case ND_NEG:
    case ND_SUB:
        return "-";
    case ND_MUL:
        return "*";
    case ND_DIV:
        return "/";
    case ND_REM:
        return "%";
    case ND_NUM:
        return "<number>";
    }

    assert(0 && "not reachable");
    return "";
}

void debug_print_token(Token const* tok)
{
    fprintf(stderr, "%s:%d:%d:[%s] '%.*s'\n",
        tok->sourceInfo->file,
        tok->line,
        tok->col,
        token_kind_to_string(tok->eTokenKind),
        tok->len,
        tok->start);
}

void debug_print_tokens(List const* toks)
{
    fprintf(stderr, "*** tokens ***\n");
    for (ListNode const* n = toks->front; n; n = n->next) {
        debug_print_token((Token const*)(n + 1));
    }
}

static void debug_print_node_internal(Node const* node, int depth)
{
    char const* const INDENT = "                                                ";
    assert(node);

    switch (node->eNodeKind) {
    case ND_NUM:
        fprintf(stderr, "%*c%d\n", depth * 2, ' ', node->val);
        break;
    case ND_NEG:
        fprintf(stderr, "%*c-\n", depth * 2, ' ');
        debug_print_node_internal(node->rhs, depth + 1);
        break;
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_REM:
        debug_print_node_internal(node->lhs, depth + 1);
        fprintf(stderr, "%.*s/\n", depth * 2, INDENT);
        fprintf(stderr, "%.*s%s\n", depth * 2, INDENT, node_kind_to_symbol(node->eNodeKind));
        fprintf(stderr, "%.*s\\\n", depth * 2, INDENT);
        debug_print_node_internal(node->rhs, depth + 1);
        break;
    case ND_INVALID:
        assert(0);
        break;
    }
}

void debug_print_node(Node const* node)
{
    fprintf(stderr, "*** AST ***\n");
    debug_print_node_internal(node, 0);
}