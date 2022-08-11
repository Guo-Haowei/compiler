#include "minic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOKSTR(TOK) (TOK)->len, (TOK)->start

// All local variable instances created during parsing are
// accumulated to this list.
static Obj* s_locals;
static Obj* s_globals;

// Scope for struct tags
typedef struct TagScope TagScope;
struct TagScope {
    TagScope* next;
    char* name;
    Type* ty;
};

// Scope for local or global variables.
typedef struct VarScope VarScope;
struct VarScope {
    VarScope* next;
    char* name;
    Obj* var;
};

// Represents a block scope.
typedef struct Scope Scope;
struct Scope {
    Scope* next;
    VarScope* vars;
    TagScope* tags;
};

static Scope _s_scope;
static Scope* s_scope = &_s_scope;

static void enter_scope(void)
{
    Scope* scope = calloc(1, sizeof(Scope));
    scope->next = s_scope;
    s_scope = scope;
}

static void leave_scope(void)
{
    s_scope = s_scope->next;
}

static VarScope* push_var_scope(char* name, Obj* var)
{
    VarScope* sc = calloc(1, sizeof(VarScope));
    sc->name = name;
    sc->var = var;
    sc->next = s_scope->vars;
    s_scope->vars = sc;
    return sc;
}

static TagScope* push_tag_scope(Token* tok, Type* ty)
{
    TagScope* sc = calloc(1, sizeof(TagScope));
    sc->name = strncopy(tok->start, tok->len);
    sc->ty = ty;
    sc->next = s_scope->tags;
    s_scope->tags = sc;
    return sc;
}

/**
 * Create Node API
 */
static Node* new_node(NodeKind eNodeKind, Token const* tok)
{
    static int s_id = 0;
    Node* node = calloc(1, sizeof(Node));
    node->id = s_id++;
    node->tok = tok;
    node->eNodeKind = eNodeKind;
    return node;
}

static Node* new_num(int64_t val, Token const* tok)
{
    Node* node = new_node(ND_NUM, tok);
    node->eNodeKind = ND_NUM;
    node->val = val;
    return node;
}

static Node* new_var(Obj* var, Token const* tok)
{
    Node* node = new_node(ND_VAR, tok);
    node->var = var;
    return node;
}

static Node* new_binary(NodeKind eNodeKind, Node* lhs, Node* rhs, Token const* tok)
{
    Node* node = new_node(eNodeKind, tok);
    node->isBinary = true;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node* new_unary(NodeKind eNodeKind, Node* expr, Token const* tok)
{
    Node* node = new_node(eNodeKind, tok);
    node->isUnary = true;
    node->lhs = expr;
    return node;
}

static Obj* new_variable(char* name, Type* type)
{
    static int s_id = 0;
    Obj* var = calloc(1, sizeof(Obj));
    var->id = s_id++;
    var->name = name;
    var->type = type;
    push_var_scope(name, var);
    return var;
}

static Obj* new_lvar(char* name, Type* type)
{
    Obj* var = new_variable(name, type);
    var->isLocal = true;
    var->next = s_locals;
    s_locals = var;
    return var;
}

static Obj* new_gvar(char* name, Type* type)
{
    Obj* var = new_variable(name, type);
    var->next = s_globals;
    s_globals = var;
    return var;
}

typedef Node* (*ParseBinaryFn)(ListNode**);

// Find a local variable by name.
static Type* find_tag(Token* tok)
{
    for (Scope* sc = s_scope; sc; sc = sc->next) {
        for (TagScope* sc2 = sc->tags; sc2; sc2 = sc2->next) {
            if (strncmp(tok->start, sc2->name, tok->len) == 0) {
                return sc2->ty;
            }
        }
    }
    return nullptr;
}

static Obj* find_var(Token const* tok)
{
    for (Scope* sc1 = s_scope; sc1; sc1 = sc1->next) {
        for (VarScope* sc2 = sc1->vars; sc2; sc2 = sc2->next) {
            if (strncmp(tok->start, sc2->name, tok->len) == 0) {
                return sc2->var;
            }
        }
    }

    return nullptr;
}

static Token* as_tok(ListNode* listnode)
{
    return (Token*)(listnode + 1);
}

static void tok_shift(ListNode** pToks)
{
    assert(pToks && *pToks && (*pToks)->next);
    *pToks = (*pToks)->next;
}

static bool tok_eq(ListNode* toks, const char* expect)
{
    // @TODO: handle unexpected EOF
    assert(toks);

    const Token* tok = as_tok(toks);
    const int expectLen = (int)strlen(expect);
    if (expectLen != tok->len) {
        return false;
    }

    return strncmp(tok->start, expect, expectLen) == 0;
}

static bool tok_consume(ListNode** pToks, const char* expect)
{
    // @TODO: handle unexpected EOF
    const bool equal = tok_eq(*pToks, expect);
    if (equal) {
        tok_shift(pToks);
        return true;
    }
    return false;
}

static void tok_expect(ListNode** pToks, char const* expect)
{
    assert(pToks && *pToks);
    Token const* token = as_tok(*pToks);
    const int expectLen = (int)strlen(expect);
    if (expectLen != token->len || (strncmp(token->start, expect, token->len) != 0)) {
        error_tok(token, "expected '%s'", expect);
    }

    *pToks = (*pToks)->next;
}

static char* new_unique_name()
{
    static int s_id = 0;
    return format(".L.anon.%d", s_id++);
}

static Obj* new_anon_gvar(Type* ty)
{
    return new_gvar(new_unique_name(), ty);
}

static Obj* new_string_literal(char* p, Type* type, Token* tok)
{
    Obj* var = new_anon_gvar(type);
    var->initData = p;
    var->tok = tok;
    return var;
}

static char* get_ident(const Token* tok)
{
    if (tok->eTokenKind != TK_IDENT) {
        error_tok(tok, "expected an identifier");
    }
    return strncopy(tok->start, tok->len);
}

static Node* parse_primary(ListNode** pToks);
static Node* parse_postfix(ListNode** pToks);
static Node* parse_unary(ListNode** pToks);
static Node* parse_mul(ListNode** pToks);
static Node* parse_add(ListNode** pToks);
static Node* parse_relational(ListNode** pToks);
static Node* parse_equality(ListNode** pToks);
static Node* parse_assign(ListNode** pToks);
static Node* parse_funccall(ListNode** pToks);
static Node* parse_expr(ListNode** pToks);
static Node* parse_expr_stmt(ListNode** pToks);
static Node* parse_compound_stmt(ListNode** pToks);
static Node* parse_decl(ListNode** pToks);
static Type* parse_declspec(ListNode** pToks);
static Type* parse_declarator(ListNode** pToks, Type* type);

static Node* new_add(Node* lhs, Node* rhs, Token* tok);
static Node* new_sub(Node* lhs, Node* rhs, Token* tok);

// primary = "(" expr ")" | "sizeof" unary | ident func-args? | str | num
static Node* parse_primary(ListNode** pToks)
{
    if (tok_consume(pToks, "(")) {
        Node* node = parse_expr(pToks);
        tok_expect(pToks, ")"); // consume ')'
        return node;
    }

    Token* tok = as_tok(*pToks);
    if (tok_consume(pToks, "sizeof")) {
        Node* node = parse_unary(pToks);
        add_type(node);
        return new_num(node->type->size, tok);
    }

    if (tok->eTokenKind == TK_NUM) {
        Node* node = new_num(tok->val, tok);
        tok_shift(pToks);
        return node;
    }

    if (tok->eTokenKind == TK_STR) {
        Obj* var = new_string_literal(tok->str, tok->type, tok);
        tok_shift(pToks);
        return new_var(var, tok);
    }

    if (tok->eTokenKind == TK_IDENT) {
        if (tok_eq((*pToks)->next, "(")) {
            return parse_funccall(pToks);
        }

        Obj* var = find_var(tok);
        if (!var) {
            // if not found, create
            error_tok(tok, "undefined variable '%.*s'", TOKSTR(tok));
        }

        tok_shift(pToks);
        return new_var(var, tok);
    }

    error_tok(tok, "expected expression before '%.*s' token", TOKSTR(tok));
    return nullptr;
}

// unary = ("+" | "-" | "*" | "&") unary
//       | postfix
static Node* parse_unary(ListNode** pToks)
{
    if (tok_consume(pToks, "+")) {
        return parse_unary(pToks);
    }

    Token* tok = as_tok(*pToks);

    if (tok_consume(pToks, "-")) {
        return new_unary(ND_NEG, parse_unary(pToks), tok);
    }

    if (tok_consume(pToks, "*")) {
        return new_unary(ND_DEREF, parse_unary(pToks), tok);
    }

    if (tok_consume(pToks, "&")) {
        return new_unary(ND_ADDR, parse_unary(pToks), tok);
    }

    return parse_postfix(pToks);
}

// struct-members = (declspec declarator (","  declarator)* ";")*
static void parse_struct_members(ListNode** pToks, Type* ty)
{
    Member head = { .next = nullptr };
    Member* cur = &head;

    while (!tok_consume(pToks, "}")) {
        Type* basety = parse_declspec(pToks);
        int i = 0;
        while (!tok_consume(pToks, ";")) {
            if (i++) {
                tok_expect(pToks, ",");
            }

            Member* member = calloc(1, sizeof(Member));
            member->type = parse_declarator(pToks, basety);
            member->name = member->type->name;
            cur = cur->next = member;
        }
    }
    ty->members = head.next;
}

// struct-decl = ident? "{" struct-members
static Type* parse_struct_decl(ListNode** pToks)
{
    // Read a struct tag.
    Token* tag = nullptr;
    Token* tok = as_tok(*pToks);
    if (tok->eTokenKind == TK_IDENT) {
        tag = tok;
        tok_shift(pToks);
    }

    if (tag && !tok_eq(*pToks, "{")) {
        Type* ty = find_tag(tag);
        if (!ty) {
            error_tok(tag, "unknown struct %.*s", TOKSTR(tag));
        }
        return ty;
    }

    tok_expect(pToks, "{");

    Type* ty = calloc(1, sizeof(Type));
    ty->eTypeKind = TY_STRUCT;
    ty->align = 1;
    parse_struct_members(pToks, ty);

    // Assign offsets within the struct to members.
    int offset = 0;
    for (Member* mem = ty->members; mem; mem = mem->next) {
        offset = align_to(offset, mem->type->align);
        mem->offset = offset;
        offset += mem->type->size;

        ty->align = MAX(ty->align, mem->type->align);
    }
    ty->size = align_to(offset, ty->align);

    // Register the struct type if a name was given.
    if (tag) {
        push_tag_scope(tag, ty);
    }
    return ty;
}

static Member* get_struct_member(Type* ty, Token* tok)
{
    for (Member* mem = ty->members; mem; mem = mem->next) {
        if (mem->name->len == tok->len && !strncmp(mem->name->start, tok->start, tok->len)) {
            return mem;
        }
    }
    error_tok(tok, "no member '%.*s'", TOKSTR(tok));
    return nullptr;
}

static Node* struct_ref(Node* lhs, Token* tok)
{
    add_type(lhs);
    if (lhs->type->eTypeKind != TY_STRUCT) {
        error_tok(lhs->tok, "not a struct");
    }

    Node* node = new_unary(ND_MEMBER, lhs, tok);
    node->member = get_struct_member(lhs->type, tok);
    return node;
}

// postfix = primary ("[" expr "]" | "." ident | "->" ident)*
static Node* parse_postfix(ListNode** pToks)
{
    Node* node = parse_primary(pToks);

    for (;;) {
        Token* tok = as_tok(*pToks);
        if (tok_consume(pToks, "[")) {
            // x[y] is short for *(x+y)
            Node* idx = parse_expr(pToks);
            tok_expect(pToks, "]");
            node = new_unary(ND_DEREF, new_add(node, idx, tok), tok);
            continue;
        }

        if (tok_consume(pToks, ".")) {
            node = struct_ref(node, as_tok(*pToks));
            tok_shift(pToks);
            continue;
        }

        if (tok_consume(pToks, "->")) {
            // x->y is short for (*x).y
            node = new_unary(ND_DEREF, node, tok);
            node = struct_ref(node, as_tok(*pToks));
            tok_shift(pToks);
            continue;
        }

        return node;
    }
}

static NodeKind to_binary_node_kind(char const* symbol)
{
#define DEFINE_NODE(NAME, BINOP, UNARYOP) \
    if (BINOP && streq(symbol, BINOP))    \
        return NAME;
#include "node.inl"
#undef DEFINE_NODE
    return ND_INVALID;
}

static Node* parse_binary_internal(ListNode** pToks, char const** symbols, ParseBinaryFn fp)
{
    Node* node = fp(pToks);

    for (;;) {
        bool found = false;
        for (char const** p = symbols; *p; ++p) {
            if (tok_consume(pToks, *p)) {
                NodeKind kind = to_binary_node_kind(*p);
                assert(kind != ND_INVALID);
                node = new_binary(kind, node, fp(pToks), as_tok(pToks[0]->prev));
                found = true;
                break;
            }
        }

        if (found) {
            continue;
        }

        return node;
    }
}

// mul = unary ("*" unary | "/" unary)*
static Node* parse_mul(ListNode** pToks)
{
    static char const* s_symbols[] = { "*", "/", "%", nullptr };
    return parse_binary_internal(pToks, s_symbols, parse_unary);
}

static Node* new_add(Node* lhs, Node* rhs, Token* tok)
{
    add_type(lhs);
    add_type(rhs);

    // num + num
    if (is_integer(lhs->type) && is_integer(rhs->type)) {
        return new_binary(ND_ADD, lhs, rhs, tok);
    }

    // both pointers
    if (lhs->type->base && rhs->type->base) {
        error_tok(tok, "invalid operands");
    }

    // swap `num + ptr` to `ptr + num`.
    if (!lhs->type->base && rhs->type->base) {
        Node* tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    // ptr + num
    rhs = new_binary(ND_MUL, rhs, new_num(lhs->type->base->size, tok), tok);
    return new_binary(ND_ADD, lhs, rhs, tok);
}

static Node* new_sub(Node* lhs, Node* rhs, Token* tok)
{
    add_type(lhs);
    add_type(rhs);

    // num - num
    if (is_integer(lhs->type) && is_integer(rhs->type)) {
        return new_binary(ND_SUB, lhs, rhs, tok);
    }

    // ptr - num
    if (lhs->type->base && is_integer(rhs->type)) {
        rhs = new_binary(ND_MUL, rhs, new_num(lhs->type->base->size, tok), tok);
        add_type(rhs);
        Node* node = new_binary(ND_SUB, lhs, rhs, tok);
        node->type = lhs->type;
        return node;
    }
    // ptr - ptr, which returns how many elements are between the two.
    if (lhs->type->base && rhs->type->base) {
        Node* node = new_binary(ND_SUB, lhs, rhs, tok);
        node->type = g_int_type;
        return new_binary(ND_DIV, node, new_num(lhs->type->base->size, tok), tok);
    }

    error_tok(tok, "invalid operands");
    return nullptr;
}

// add = mul ("+" mul | "-" mul)*
static Node* parse_add(ListNode** pToks)
{
    Node* node = parse_mul(pToks);
    Token* start = as_tok(*pToks);

    for (;;) {
        if (tok_consume(pToks, "+")) {
            node = new_add(node, parse_mul(pToks), start);
            continue;
        }

        if (tok_consume(pToks, "-")) {
            node = new_sub(node, parse_mul(pToks), start);
            continue;
        }

        return node;
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node* parse_relational(ListNode** pToks)
{
    static char const* s_symbols[] = { "<", "<=", ">", ">=", nullptr };
    return parse_binary_internal(pToks, s_symbols, parse_add);
}

// equality = relational ("==" relational | "!=" relational)*
static Node* parse_equality(ListNode** pToks)
{
    static char const* s_symbols[] = { "==", "!=", nullptr };
    return parse_binary_internal(pToks, s_symbols, parse_relational);
}

// assign = equality ("=" assign)?
static Node* parse_assign(ListNode** pToks)
{
    Node* node = parse_equality(pToks);
    if (tok_consume(pToks, "=")) {
        node = new_binary(ND_ASSIGN, node, parse_assign(pToks), as_tok(pToks[0]->prev));
    }
    return node;
}

// funcall = ident "(" (assign ("," assign)*)? ")"
static Node* parse_funccall(ListNode** pToks)
{
    const Token* funcname = as_tok(*pToks);
    tok_shift(pToks);
    tok_expect(pToks, "(");
    Node head = { .next = nullptr };
    Node* cur = &head;
    int argc = 0;
    for (; !tok_consume(pToks, ")"); ++argc) {
        if (argc) {
            tok_expect(pToks, ",");
        }
        cur = cur->next = parse_assign(pToks);
    }
    Node* node = new_node(ND_FUNCCALL, funcname);
    node->funcname = strncopy(funcname->start, funcname->len);
    node->args = head.next;
    node->argc = argc;
    return node;
}

// expr = assign ("," expr)?
static Node* parse_expr(ListNode** pToks)
{
    Node* node = parse_assign(pToks);

    Token* tok = as_tok(*pToks);
    if (tok_consume(pToks, ",")) {
        return new_binary(ND_COMMA, node, parse_expr(pToks), tok);
    }

    return node;
}

// expr-stmt = expr? ";"
static Node* parse_expr_stmt(ListNode** pToks)
{
    Token const* tok = as_tok(*pToks);

    if (tok_consume(pToks, ";")) {
        return new_node(ND_BLOCK, tok);
    }
    Node* node = new_unary(ND_EXPR_STMT, parse_expr(pToks), tok);
    tok_expect(pToks, ";");
    return node;
}

// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "for" "(" expr-stmt expr? ";" expr? ")" stmt
//      | "while" "(" expr ")" stmt
//      | "{" compound-stmt "}"
//      | expr-stmt
static Node* parse_stmt(ListNode** pToks)
{
    Token const* tok = as_tok(*pToks);

    if (tok_consume(pToks, "return")) {
        Node* node = new_unary(ND_RETURN, parse_expr(pToks), tok);
        tok_expect(pToks, ";");
        return node;
    }

    if (tok_consume(pToks, "if")) {
        Node* node = new_node(ND_IF, tok);
        tok_expect(pToks, "(");
        node->cond = parse_expr(pToks);
        tok_expect(pToks, ")");
        node->then = parse_stmt(pToks);
        if (tok_consume(pToks, "else")) {
            node->els = parse_stmt(pToks);
        }
        return node;
    }

    if (tok_consume(pToks, "for")) {
        Node* node = new_node(ND_FOR, tok);

        tok_expect(pToks, "(");
        node->init = parse_expr_stmt(pToks);
        if (!tok_consume(pToks, ";")) {
            node->cond = parse_expr(pToks);
            tok_expect(pToks, ";");
        }
        if (!tok_consume(pToks, ")")) {
            node->inc = parse_expr(pToks);
            tok_expect(pToks, ")");
        }

        node->then = parse_stmt(pToks);
        return node;
    }

    if (tok_consume(pToks, "while")) {
        Node* node = new_node(ND_FOR, tok);
        tok_expect(pToks, "(");
        node->cond = parse_expr(pToks);
        tok_expect(pToks, ")");
        node->then = parse_stmt(pToks);
        return node;
    }

    if (tok_consume(pToks, "{")) {
        return parse_compound_stmt(pToks);
    }

    return parse_expr_stmt(pToks);
}

static bool is_typename(ListNode* tok)
{
    static const char* s_types[] = {
        "char","int", "long", "short", "struct", "union"
    };

    for (size_t i = 0; i < ARRAY_COUNTER(s_types); ++i) {
        if (tok_eq(tok, s_types[i])) {
            return true;
        }
    }

    return false;
}

// compound-stmt = (declaration | stmt)* "}"
static Node* parse_compound_stmt(ListNode** pToks)
{
    Token* tok = as_tok(pToks[0]->prev);
    Node head = { .next = nullptr };
    Node* cur = &head;

    enter_scope();

    while (!tok_consume(pToks, "}")) {
        if (is_typename(*pToks)) {
            cur = cur->next = parse_decl(pToks);
        } else {
            cur = cur->next = parse_stmt(pToks);
        }
        add_type(cur);
    }

    leave_scope();

    Node* node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    return node;
}

// declspec = "char" | "short" | "int" | "long" | struct-decl | union-decl
static Type* parse_declspec(ListNode** pToks)
{
    if (tok_consume(pToks, "char")) {
        return g_char_type;
    }

    if (tok_consume(pToks, "short")) {
        return g_short_type;
    }

    if (tok_consume(pToks, "int")) {
        return g_int_type;
    }

    if (tok_consume(pToks, "long")) {
        return g_long_type;
    }

    if (tok_consume(pToks, "struct")) {
        return parse_struct_decl(pToks);
    }

    Token* tok = as_tok(*pToks);
    error_tok(tok, "expect type specifier, got '%.*s'", TOKSTR(tok));
    return nullptr;
}

// type-suffix = ("(" func-params? ")")?
// func-params = param ("," param)*
// param       = declspec declarator
static Type* parse_func_params(ListNode** pToks, Type* type)
{
    Type head = { .next = nullptr };
    Type* cur = &head;
    while (!tok_consume(pToks, ")")) {
        if (cur != &head) {
            tok_expect(pToks, ",");
        }
        Type* basety = parse_declspec(pToks);
        Type* ty = parse_declarator(pToks, basety);
        cur = cur->next = copy_type(ty);
    }
    type = func_type(type);
    type->params = head.next;
    return type;
}

static int64_t get_number(Token* tok)
{
    if (tok->eTokenKind != TK_NUM) {
        error_tok(tok, "expected a number");
    }
    return  tok->val;
}

// type-suffix = "(" func-params
//             | "[" num "]" type-suffix
//             | ε
static Type* parse_type_suffix(ListNode** pToks, Type* type)
{
    if (tok_consume(pToks, "(")) {
        return parse_func_params(pToks, type);
    }

    if (tok_consume(pToks, "[")) {
        int arrayLen = (int)get_number(as_tok(*pToks));
        tok_shift(pToks);
        tok_expect(pToks, "]");
        type = parse_type_suffix(pToks, type);
        return array_of(type, arrayLen);
    }

    return type;
}

// declarator = "*"* ident type-suffix
static Type* parse_declarator(ListNode** pToks, Type* type)
{
    while (tok_consume(pToks, "*")) {
        type = pointer_to(type);
    }

    Token* tok = as_tok(*pToks);
    if (tok->eTokenKind != TK_IDENT) {
        error_tok(tok, "expected a variable name");
    }

    tok_shift(pToks);
    type = parse_type_suffix(pToks, type);
    type->name = tok;
    return type;
}

// declaration = declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
static Node* parse_decl(ListNode** pToks)
{
    Type* base_type = parse_declspec(pToks);
    Node head = { .next = nullptr };
    Node* cur = &head;
    int i = 0;
    while (!tok_consume(pToks, ";")) {
        if (i++ > 0) {
            tok_expect(pToks, ",");
        }

        Type* type = parse_declarator(pToks, base_type);
        Obj* var = new_lvar(get_ident(type->name), type);

        if (tok_consume(pToks, "=")) {
            const Token* eqTok = as_tok(pToks[0]->prev);
            Node* lhs = new_var(var, type->name);
            Node* rhs = parse_assign(pToks);
            Node* node = new_binary(ND_ASSIGN, lhs, rhs, eqTok);
            cur = cur->next = new_unary(ND_EXPR_STMT, node, lhs->tok);
        }
    }

    Node* node = new_node(ND_BLOCK, as_tok(*pToks));
    node->body = head.next;
    return node;
}

static void create_param_lvars(Type* param)
{
    if (param) {
        create_param_lvars(param->next);
        new_lvar(get_ident(param->name), param);
    }
}

static void parse_global_variable(ListNode** pToks, Type* basety)
{
    int cnt = 0;
    while (!tok_consume(pToks, ";")) {
        if (cnt) {
            tok_expect(pToks, ",");
        }

        Type* type = parse_declarator(pToks, basety);
        new_gvar(get_ident(type->name), type);
        ++cnt;
    }
}

static Obj* parse_function(ListNode** pToks, Type* basetpye)
{
    Type* type = parse_declarator(pToks, basetpye);
    Obj* fn = new_gvar(get_ident(type->name), type);
    fn->isFunc = true;
    s_locals = nullptr;

    enter_scope();

    create_param_lvars(type->params);
    fn->params = s_locals;
    tok_expect(pToks, "{");
    fn->body = parse_compound_stmt(pToks);
    fn->locals = s_locals;

    leave_scope();

    return fn;
}

// Lookahead tokens and returns true if a given token is a start
// of a function definition or declaration.
static bool is_function(ListNode** pToks)
{
    if (tok_eq(*pToks, ";")) {
        return false;
    }

    Type dummy;
    Type* type = parse_declarator(pToks, &dummy);
    return type->eTypeKind == TY_FUNC;
}

// program = (function-definition | global-variable)*
Obj* parse(List* toks)
{
    s_globals = NULL;
    ListNode* iter = toks->front;
    while (as_tok(iter)->eTokenKind != TK_EOF) {
        Type* basetype = parse_declspec(&iter);

        ListNode* dummy = iter;
        // do not pass the actual iter
        // because we only need to peek ahead to find out if object is a function or not
        if (is_function(&dummy)) {
            parse_function(&iter, basetype);
            continue;
        }

        parse_global_variable(&iter, basetype);
    }
    return s_globals;
}
