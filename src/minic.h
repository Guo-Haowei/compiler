#ifndef __MINIC_H__
#define __MINIC_H__
#include <assert.h>

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

#define unreachable() assert(0)

// utilities
#define ARRAY_COUNTER(arr) (sizeof(arr) / sizeof(*(arr)))
#define STATIC_ASSERT(COND) typedef char _static_assertion_[(COND) ? 1 : -1]

#define assertindex(a, bound) assert(((int)a >= 0) && ((int)a < (int)bound))
#define align_to(x, a) (((x) + (a)-1) & ~((a)-1))

typedef enum token_kind_t {
#define DEFINE_TOKEN(NAME) NAME,
#include "token.inl"
#undef DEFINE_TOKEN
    TK_COUNT,
} TokenKind;

typedef enum node_kind_t {
#define DEFINE_NODE(NAME, BINOP, UNARYOP) NAME,
#include "node.inl"
#undef DEFINE_NODE
    ND_COUNT,
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

// Local variable
typedef struct obj_t Obj;
struct obj_t {
    Obj* next;
    char const* name; // Variable name
    int offset;       // Offset from RBP
};

// AST node
typedef struct node_t Node;
struct node_t {
    NodeKind eNodeKind;
    Node* next;
    Node* lhs;
    Node* rhs;
    Obj* var; // Used if eNodeKind == ND_VAR
    int val;  // Used if eNodeKind == ND_NUM

    // "if" or "for" statement
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;

    // {...} block statement
    Node* body;

    // @TODO: remove flags
    int isBinary;
    int isUnary;
};

// Function
typedef struct function_t {
    Node* body;
    Obj* locals;
    int stackSize;
} Function;

typedef struct lexer_t {
    SourceInfo const* sourceInfo;
    char const* p;
    int line;
    int col;
} Lexer;

int token_as_int(Token const* tok);

typedef Node* (*ParseBinaryFn)(ListNode**);

List* lex(SourceInfo const* sourceInfo);
Function* parse(List* toks);
void gen(Function const* prog);

void error_at_lexer(Lexer const* lexer, char const* const fmt, ...);
void error_at_token(Token const* token, char const* const fmt, ...);

// DEBUG
char const* token_kind_to_string(TokenKind eTokenKind);
char const* node_kind_to_string(NodeKind eNodeKind);

void debug_print_token(Token const* tok);
void debug_print_tokens(List const* toks);
void debug_print_node(Node const* node);

// @TODO: implement
void debug_validate_node(Node const* node);

// utils
char* strnduplicate(char const* const src, int n);

#endif
