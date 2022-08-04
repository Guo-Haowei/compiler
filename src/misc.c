#include "minic.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EMPTYLINE "                                                                                "
#define UNDERLINE "^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"

static void verror_at(char const* file, char const* source, int sourceLen, int line, int col, int span, char const* const fmt, va_list args)
{
    assert(file);
    assert(source);
    assert(sourceLen);
    assert(line);
    assert(col);
    assert(span);

    // fprintf(stderr, "debug verror_at(): span: %d\n", span);

    fprintf(stderr, "%s:%d:%d: error: ", file, line, col);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

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
    fprintf(stderr, "%5d | %.*s\n", line, lineLen, lineStart);

    fprintf(stderr, "      |%.*s%.*s\n", col, EMPTYLINE, span, UNDERLINE);

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
    int span = tok->len;
    if (tok->eTokenKind == TK_EOF) {
        span = 1;
    }

    va_list args;
    va_start(args, fmt);
    verror_at(file, source, sourceLen, line, col, span, fmt, args);
    va_end(args);
}

char const* token_kind_to_string(TokenKind eTokenKind)
{
    switch (eTokenKind) {
    case TK_IDENT:
        return "TK_IDENT";
    case TK_PUNCT:
        return "TK_PUNCT";
    case TK_NUM:
        return "TK_NUM";
    case TK_EOF:
        return "TK_EOF";
    }

    unreachable();
    return "";
}

char const* node_kind_to_string(NodeKind eNodeKind)
{
    assertindex(eNodeKind, ND_COUNT);

    static char const* const s_names[] = {
#define DEFINE_NODE(NAME) #NAME,
#include "node.inl"
#undef DEFINE_NODE
    };
    STATIC_ASSERT(ARRAY_COUNTER(s_names) == ND_COUNT);

    return s_names[eNodeKind];
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
    assert(node);

    if (node->eNodeKind == ND_NUM) {
        fprintf(stderr, "%d", node->val);
        return;
    }

    if (node->eNodeKind == ND_VAR) {
        fprintf(stderr, "%c", node->name);
        return;
    }

    if (node->isBinary) {
        fprintf(stderr, "([binary %s] ", node_kind_to_string(node->eNodeKind));
        debug_print_node_internal(node->lhs, depth);
        fprintf(stderr, ", ");
        debug_print_node_internal(node->rhs, depth);
        fprintf(stderr, " )");
        return;
    }

    if (node->isUnary) {
        fprintf(stderr, "([unary %s] ", node_kind_to_string(node->eNodeKind));
        debug_print_node_internal(node->rhs, depth);
        fprintf(stderr, " )");
        return;
    }

    unreachable();
}

void debug_print_node(Node const* node)
{
    fprintf(stderr, "*** AST ***\n");
    for (Node const* n = node; n; n = n->next) {
        debug_print_node_internal(n, 0);
    }
    fprintf(stderr, "\n");
}