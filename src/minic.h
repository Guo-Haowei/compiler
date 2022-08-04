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
    ND_NEG,     // -
    ND_NUM,     // Integer
} NodeKind;

typedef struct source_info_t {
    char const* file;
    char const* start;
    char const* end;
    int len;
} SourceInfo;

typedef struct token_t {
    TokenKind eTokenKind;
    int line;
    int col;
    char const* start;
    char const* end;
    int len;

    SourceInfo const* sourceInfo;
} Token;

typedef struct node_t {
    NodeKind eNodeKind;
    struct node_t* lhs;
    struct node_t* rhs;
    int val;
} Node;

typedef struct lexer_t {
    SourceInfo const* sourceInfo;
    char const* p;
    int line;
    int col;
} Lexer;

int token_as_int(Token const* token);

List* lex(SourceInfo const* sourceInfo);
Node* parse(List* tokens);
void gen(Node const* node);

void error_at_lexer(Lexer const* lexer, char const* const fmt, ...);
void error_at_token(Token const* token, char const* const fmt, ...);

// DEBUG
char const* token_kind_to_string(TokenKind eTokenKind);
char const* node_kind_to_string(NodeKind eNodeKind);

void debug_print_token(Token const* tok);
void debug_print_tokens(List const* toks);
void debug_print_node(Node const* node);

#endif
