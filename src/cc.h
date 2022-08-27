#ifndef __CC_H__
#define __CC_H__
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "generic/array.h"
#include "generic/list.h"

#include "utility.h"

/**
 * file_cache.c
 */
Array* fcache_get(char* absPath);
bool fcache_add(char* absPath, Array* toks);

typedef enum token_kind_t {
    TK_IDENT,   // Identifiers
    TK_PUNCT,   // Punctuators
    TK_KEYWORD, // Keywords
    TK_NUM,     // Numeric literals
    TK_STR,     // String literals
    TK_EOF,     // End-of-file markers
    TK_COUNT,
} TokenKind;

typedef enum node_kind_t {
    ND_INVALID,
    ND_NULL_EXPR, // null expr
    ND_ADD,       // +
    ND_SUB,       // -
    ND_MUL,       // *
    ND_DIV,       // /
    ND_MOD,       // %
    ND_NEG,       // unary -
    ND_ADDR,      // unary &
    ND_DEREF,     // unary *
    ND_NOT,       // unary !
    ND_BITNOT,    // unary ~
    ND_EQ,        // ==
    ND_NE,        // !=
    ND_LT,        // <
    ND_LE,        // <=
    ND_BITAND,    // &
    ND_BITOR,     // |
    ND_BITXOR,    // |
    ND_LOGAND,    // &&
    ND_LOGOR,     // ||
    ND_SHL,       // <<
    ND_SHR,       // >>
    ND_ASSIGN,    // =
    ND_TERNARY,   // ?:
    ND_COMMA,     // ,
    ND_MEMBER,    // . (struct member access)
    ND_IF,        // "if"
    ND_FOR,       // "for" or "while"
    ND_RETURN,    // "return"
    ND_BLOCK,     // { ... }
    ND_GOTO,      // "goto"
    ND_LABEL,     // label
    ND_SWITCH,    // "switch"
    ND_CASE,      // "case"
    ND_FUNCCALL,  // function call
    ND_EXPR_STMT, // expression statement
    ND_VAR,       // variable
    ND_NUM,       // number literal
    ND_CAST,      // cast
    ND_MEMZERO,   // zero-clear a stack variable
    ND_COUNT,
} NodeKind;

typedef struct source_info_t {
    char file[MAX_OSPATH];
    char* start;
    char* end;
    int len;
} SourceInfo;

typedef struct Node Node;
typedef struct Type Type;
typedef struct Obj Obj;
typedef struct Member Member;

typedef struct Token Token;
struct Token {
    TokenKind kind;
    int line;
    int col;

    char* p;
    char* raw;
    int len;

    SourceInfo* sourceInfo;

    int64_t val;
    Type* type;
    char* str;

    int isFirstTok;
    Token* expandedFrom;
};

// Variable or function
struct Obj {
    int id;
    char* name;
    Obj* next;
    Type* type;

    int isLocal;
    int isFunc;
    int isDefinition;
    int isStatic;

    // variable
    int offset;

    // function
    Obj* params;
    Node* body;
    Obj* locals;
    Obj* vaArea;
    int stackSize;

    // Global variable
    char* initData;
    Token* tok;
};

// AST node
struct Node {
    NodeKind kind;
    Node* next;
    Type* type;
    Node* lhs;
    Node* rhs;
    Obj* var;    // Used if kind == ND_VAR
    int64_t val; // Used if kind == ND_NUM

    Token* tok; // Representative token

    // "if" or "for" statement
    Node* cond;
    Node* then;
    Node* els;
    Node* init;
    Node* inc;

    // {...} block statement
    Node* body;

    // Struct member access
    Member* member;

    // Function call
    char* funcname;
    Array* args;

    // Goto or labeled statement
    char* label;
    char* uniqueLabel;
    Node* gotoNext;

    // "break" or "continue"
    char* brkLabel;
    char* cntLabel;

    // Switch-cases
    Node* caseNext;
    Node* caseDefault;

    // @TODO: remove flags
    int isBinary;
    int isUnary;
};

typedef struct lexer_t {
    SourceInfo* sourceInfo;
    char* p;
    int line;
    int col;
} Lexer;

// type
typedef enum {
    TY_INVALID,
    TY_VOID,
    TY_CHAR,
    TY_SHORT,
    TY_INT,
    TY_LONG,
    TY_PTR,
    TY_FUNC,
    TY_ARRAY,
    TY_STRUCT,
    TY_UNION,
    TY_ENUM,
} TypeKind;

struct Type {
    TypeKind kind;
    int size;        // sizeof() value
    int align;       // alignment
    bool isUnsigned; // unsigned

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
    Member* members;

    // function
    Type* retType;
    // @TODO: make it array instead
    Type* params;
    Type* next;
    int isVariadic;
};

// Struct member
struct Member {
    Member* next;
    Type* type;
    Token* name;
    int idx;
    int offset;
};

extern Type g_void_type;
extern Type g_char_type;
extern Type g_short_type;
extern Type g_int_type;
extern Type g_long_type;
extern Type g_uchar_type;
extern Type g_ushort_type;
extern Type g_uint_type;
extern Type g_ulong_type;

bool is_integer(Type* type);
Type* copy_type(Type* type);
Type* pointer_to(Type* base);
Type* func_type(Type* returnType);
Type* array_of(Type* base, int size);
Type* struct_type();
void add_type(Node* node);
Type* enum_type();

bool is_token_equal(Token* token, char* symbol);

List* preproc(Array* toks, char* includepath);

void dump_preproc(List* toks);
Node* new_cast(Node* expr, Type* type, Token* tok);
Obj* parse(List* toks);

void gen(Obj* prog, char* srcname, char* asmname);

/**
 * lexer.c
 */
Array* lex(char* filename);

enum {
    LEVEL_NOTE,
    LEVEL_WARN,
    LEVEL_ERROR,
};

void _error(char* msg);
void _error_lex(Lexer* lexer, char* msg);
void _error_tok(int level, Token* token, char* msg);
void _info_tok(Token* tok, char* msg);

#define error(...)                               \
    {                                            \
        char buf[256];                           \
        snprintf(buf, sizeof(buf), __VA_ARGS__); \
        _error(buf);                             \
    }                                            \
    (void)0

#define error_lex(LEXER, ...)                    \
    {                                            \
        char buf[256];                           \
        snprintf(buf, sizeof(buf), __VA_ARGS__); \
        _error_lex(LEXER, buf);                  \
    }                                            \
    (void)0

#define error_tok(TOK, ...)                      \
    {                                            \
        char buf[256];                           \
        snprintf(buf, sizeof(buf), __VA_ARGS__); \
        _error_tok(LEVEL_ERROR, TOK, buf);       \
    }                                            \
    (void)0

#define warn_tok(TOK, ...)                       \
    {                                            \
        char buf[256];                           \
        snprintf(buf, sizeof(buf), __VA_ARGS__); \
        _error_tok(LEVEL_WARN, TOK, buf);        \
    }                                            \
    (void)0

#define info_tok(TOK, ...)                       \
    {                                            \
        char buf[256];                           \
        snprintf(buf, sizeof(buf), __VA_ARGS__); \
        _info_tok(TOK, buf);                     \
    }                                            \
    (void)0

/**
 *  utility
 */

typedef struct {
    List* tokens;
    ListNode* cursor;
} TokenReader;

Token* tr_peek_n(TokenReader* reader, int n);
Token* tr_peek(TokenReader* reader);
Token* tr_read(TokenReader* reader);
bool tr_equal(TokenReader* reader, char* symbol);
bool tr_consume(TokenReader* reader, char* symbol);
void tr_expect(TokenReader* reader, char* symbol);

int unique_id();

#endif
