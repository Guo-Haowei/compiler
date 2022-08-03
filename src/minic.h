#ifndef __MINIC_H__
#define __MINIC_H__
#include "list.h"

typedef int bool;

#ifndef false
#define false 0
#endif

#ifndef true
#define true 1
#endif

#ifndef nullptr
#define nullptr ((void*)0)
#endif

typedef enum token_kind_t {
    TK_PUNCT,
    TK_NUM,
    TK_EOF,
} TokenKind;

typedef enum node_kind_t {
    ND_INVALID, // invalid
    ND_ADD,     // +
    ND_SUB,     // -
    ND_MUL,     // *
    ND_DIV,     // /
    ND_REM,     // %
    ND_NUM,     // Integer
} NodeKind;

typedef struct token_t {
    TokenKind eTokenKind;
    int line;
    int col;
    const char* start;
    const char* end;
    int len;
} Token;

typedef struct node_t {
    NodeKind eNodeKind;
    struct node_t* lhs;
    struct node_t* rhs;
    int val;
} Node;

typedef struct lexer_t {
    const char* file;
    const char* source;
    int sourceLen;
    const char* p;
    int line;
    int col;
} Lexer;

int token_as_int(Token const* token);

List* lex(const char* source);
Node* parse(List* tokens);

void error_at(const Lexer* lexer, char const* const fmt, ...);

// DEBUG
char const* token_kind_to_string(TokenKind eTokenKind);
char const* node_kind_to_string(NodeKind eNodeKind);

void debug_print_token(Token const* token);
void debug_print_tokens(List const* tokens);
void debug_print_node(Node const* node);

#endif
