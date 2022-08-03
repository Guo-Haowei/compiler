#include "minic.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// @TODO: refactor
void error_at(const Lexer* lexer, char const* const fmt, ...)
{
    const char* file = lexer->file ? lexer->file : "<unknown>";
    const int line = lexer->line;
    const int col = lexer->col;

    // printf("line %d, col: %d, offset: %d, char: '%c'\n", line, col, (int)(lexer->p - lexer->source), *lexer->p);

    printf("%s:%d:%d: error: ", file, line, col);

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");

    // print line
    const char* lineStart = lexer->source;
    for (int curLine = 1; curLine < line; ++curLine) {
        lineStart = strchr(lineStart, '\n');
        assert(lineStart);
        ++lineStart;
    }

    const char* lineEnd = lineStart;
    if ((lineEnd = strchr(lineEnd, '\n')) == nullptr) {
        lineEnd = lexer->source + lexer->sourceLen;
    }

    const int lineLen = lineEnd - lineStart;
    printf("%5d | %.*s\n", line, lineLen, lineStart);

    // print underscore
    printf("      |%*c^\n", col, ' ');

    exit(-1);
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

void debug_print_token(Token const* token)
{
    fprintf(stderr, "<%s> '%.*s'(%d:%d)\n", token_kind_to_string(token->eTokenKind), token->len, token->start, token->line, token->col);
}

void debug_print_tokens(List const* tokens)
{
    fprintf(stderr, "*** tokens ***\n");
    for (ListNode const* n = tokens->front; n; n = n->next) {
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