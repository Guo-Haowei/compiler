#ifndef __MINIC_H__
#define __MINIC_H__
#include <assert.h>

#include "list.h"

typedef int bool;
typedef unsigned int uint;

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

typedef struct node_t Node;
typedef struct type_t Type;
typedef struct function_t Function;
typedef struct obj_t Obj;

// Local variable
struct obj_t {
    uint id;
    Obj* next;
    char const* name; // Variable name
    Type* type;       // Type
    int offset;       // Offset from RBP
};

// AST node
struct node_t {
    uint id;
    NodeKind eNodeKind;
    Node* next;
    Type* type;
    Node* lhs;
    Node* rhs;
    Obj* var; // Used if eNodeKind == ND_VAR
    int val;  // Used if eNodeKind == ND_NUM

    Token const* tok; // Representative token

    // "if" or "for" statement
    Node* cond;
    Node* then;
    Node* els;
    Node* init;
    Node* inc;

    // {...} block statement
    Node* body;

    // Function call
    char* funcname;
    Node* args;
    int argc;

    // @TODO: remove flags
    int isBinary;
    int isUnary;
};

// Function
struct function_t {
    Function* next;
    char* name;
    Obj* params;

    Node* body;
    Obj* locals;
    int stackSize;
};

typedef struct lexer_t {
    SourceInfo const* sourceInfo;
    char const* p;
    int line;
    int col;
} Lexer;

int token_as_int(Token const* tok);

// type
typedef enum {
    TY_INT,
    TY_PTR,
    TY_FUNC,
    TY_ARRAY,
} TypeKind;

struct type_t {
    TypeKind eTypeKind;
    int size; // sizeof() value

    // Pointer-to or array-of type. We intentionally use the same member
    // to represent pointer/array duality in C.
    //
    // In many contexts in which a pointer is expected, we examine this
    // member instead of "kind" member to determine whether a type is a
    // pointer or not. That means in many contexts "array of T" is
    // naturally handled as if it were "pointer to T", as required by
    // the C spec.
    Type* base;
    Token* name;

    // Array
    int arrayLen;

    // function
    Type* retType;
    Type* params;
    Type* next;
};

extern Type* g_int_type;

bool is_integer(Type* type);
Type* copy_type(Type* type);
Type* pointer_to(Type* base);
Type* func_type(Type* returnType);
Type* array_of(Type* base, int size);
void add_type(Node* node);

List* lex(SourceInfo const* sourceInfo);
Function* parse(List* toks);
void gen(Function* prog);

void error_lex(Lexer const* lexer, char const* const fmt, ...);
void error_tok(Token const* token, char const* const fmt, ...);

// error
char const* token_kind_to_string(TokenKind eTokenKind);
char const* node_kind_to_string(NodeKind eNodeKind);

// DEBUG
void debug_print_token(Token const* tok);
void debug_print_tokens(List const* toks);
void debug_print_node(Node const* node);

// @TODO: implement
void debug_validate_node(Node const* node);

// utils
char* strnduplicate(char const* const src, int n);

#endif
