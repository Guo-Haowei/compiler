#ifndef __MINIC_H__
#define __MINIC_H__
#include <assert.h>
#include <stdint.h>

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
#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)

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

typedef struct Node Node;
typedef struct Type Type;
typedef struct Obj Obj;
typedef struct Member Member;

typedef struct Token {
    TokenKind eTokenKind;
    int line;
    int col;
    char const* start;
    char const* end;
    int len;

    const SourceInfo* sourceInfo;

    // int
    int64_t val;

    // string
    Type* type;
    char* str;
} Token;

// Variable or function
struct Obj {
    uint id;
    char* name;
    Obj* next;
    Type* type;

    bool isLocal;
    bool isFunc;

    // variable
    int offset;

    // function
    Obj* params;
    Node* body;
    Obj* locals;
    int stackSize;

    // Global variable
    char* initData;
    Token* tok;
};

// AST node
struct Node {
    uint id;
    NodeKind eNodeKind;
    Node* next;
    Type* type;
    Node* lhs;
    Node* rhs;
    Obj* var; // Used if eNodeKind == ND_VAR
    int64_t val;  // Used if eNodeKind == ND_NUM

    Token const* tok; // Representative token

    // "if" or "for" statement
    Node* cond;
    Node* then;
    Node* els;
    Node* init;
    Node* inc;

    // {...} block statement
    Node* body;

    // Struct member access
    Member *member;

    // Function call
    char* funcname;
    Node* args;
    int argc;

    // @TODO: remove flags
    int isBinary;
    int isUnary;
};

typedef struct lexer_t {
    const SourceInfo* sourceInfo;
    const char* p;
    int line;
    int col;
} Lexer;

// type
typedef enum {
    TY_INVALID,
    TY_CHAR,
    TY_INT,
    TY_SHORT,
    TY_LONG,
    TY_PTR,
    TY_FUNC,
    TY_ARRAY,
    TY_STRUCT,
    TY_UNION,
} TypeKind;

struct Type {
    TypeKind eTypeKind;
    int size;  // sizeof() value
    int align; // alignment

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

    // Struct
    Member *members;

    // function
    Type* retType;
    Type* params;
    Type* next;
};

// Struct member
struct Member {
    Member *next;
    Type *type;
    Token *name;
    int offset;
};

extern Type* g_char_type;
extern Type* g_short_type;
extern Type* g_int_type;
extern Type* g_long_type;

bool is_integer(Type* type);
Type* copy_type(Type* type);
Type* pointer_to(Type* base);
Type* func_type(Type* returnType);
Type* array_of(Type* base, int size);
void add_type(Node* node);

List* lex_file(const char* filename);
Obj* parse(List* toks);
void gen(Obj* prog, const char* inputName);

void error(const char* const fmt, ...);
void error_lex(const Lexer* lexer, const char* const fmt, ...);
void error_tok(const Token* token, const char* const fmt, ...);

/**
 * misc
 */
const char* token_kind_to_string(TokenKind eTokenKind);
const char* node_kind_to_string(NodeKind eNodeKind);

// DEBUG
void debug_print_token(Token const* tok);
void debug_print_tokens(List const* toks);

/**
 * string
 */
bool streq(const char* a, const char* b);
char* strncopy(const char* src, int n);
char* format(const char* fmt, ...);

#endif
