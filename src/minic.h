#ifndef __MINIC_H__
#define __MINIC_H__
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

    bool isFirstTok;
    Token* expandedFrom;
};

// Variable or function
struct Obj {
    int id;
    char* name;
    Obj* next;
    Type* type;

    bool isLocal;
    bool isFunc;
    bool isDefinition;
    bool isStatic;

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
    uint32_t id;
    NodeKind eNodeKind;
    Node* next;
    Type* type;
    Node* lhs;
    Node* rhs;
    Obj* var;    // Used if eNodeKind == ND_VAR
    int64_t val; // Used if eNodeKind == ND_NUM

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
    Node* args;
    int argc;

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
#define DEFINE_BASE_TYPE(name, kind, sz, al) kind,
#include "base_type.inl"
#undef DEFINE_BASE_TYPE
    TY_PTR,
    TY_FUNC,
    TY_ARRAY,
    TY_STRUCT,
    TY_UNION,
    TY_ENUM,
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
    Member* members;

    // function
    Type* retType;
    Type* params;
    Type* next;
    bool isVariadic;
};

// Struct member
struct Member {
    Member* next;
    Type* type;
    Token* name;
    int offset;
};

extern Type* g_void_type;
extern Type* g_char_type;
extern Type* g_short_type;
extern Type* g_int_type;
extern Type* g_long_type;

bool is_integer(Type* type);
Type* copy_type(Type* type);
Type* pointer_to(Type* base);
Type* func_type(Type* returnType);
Type* array_of(Type* base, int size);
Type* struct_type();
void add_type(Node* node);
Type* enum_type();

bool is_token_equal(Token* token, char* symbol);

Array* lex(char* filename);

List* preproc(Array* toks, char* includepath);

void dump_preproc(List* toks);
Node* new_cast(Node* expr, Type* type, Token* tok);
Obj* parse(List* toks);

void gen(Obj* prog, char* srcname, char* asmname);

void error(char* fmt, ...);
void error_lex(Lexer* lexer, char* fmt, ...);
void error_tok(Token* token, char* fmt, ...);
void warn_tok(Token* token, char* fmt, ...);
void info_tok(Token* tok, char* fmt, ...);

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

/**
 * misc
 */
char* token_kind_to_string(TokenKind eTokenKind);
char* format(char* fmt, ...);
char* read_file(char* path);

// DEBUG
void debug_print_token(Token* tok);
void debug_print_tokens(List* toks);

#endif
