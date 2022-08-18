#include "minic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    TokenReader reader;
} ParserState;

/// token stream
static Token* peek_n(ParserState* state, int n)
{
    return tr_peek_n(&(state->reader), n);
}

static Token* peek(ParserState* state)
{
    return peek_n(state, 0);
}

static Token* read(ParserState* state)
{
    return tr_read(&(state->reader));
}

static bool equal(ParserState* state, const char* symbol)
{
    return tr_equal(&(state->reader), symbol);
}

static bool consume(ParserState* state, const char* symbol)
{
    return tr_consume(&(state->reader), symbol);
}

static void expect(ParserState* state, const char* symbol)
{
    tr_expect(&(state->reader), symbol);
}

// All local variable instances created during parsing are
// accumulated to this list.
static Obj* s_locals;
static Obj* s_globals;

// Scope for struct or union tags
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
    sc->name = strncopy(tok->p, tok->len);
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

typedef Node* (*ParseBinaryFn)(ParserState*);

// Find a local variable by name.
static Type* find_tag(Token* tok)
{
    for (Scope* sc = s_scope; sc; sc = sc->next) {
        for (TagScope* sc2 = sc->tags; sc2; sc2 = sc2->next) {
            if (strcmp(tok->raw, sc2->name) == 0) {
                return sc2->ty;
            }
        }
    }
    return NULL;
}

static Obj* find_var(Token const* tok)
{
    for (Scope* sc1 = s_scope; sc1; sc1 = sc1->next) {
        for (VarScope* sc2 = sc1->vars; sc2; sc2 = sc2->next) {
            if (strcmp(tok->raw, sc2->name) == 0) {
                return sc2->var;
            }
        }
    }
    return NULL;
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
    if (tok->kind != TK_IDENT) {
        error_tok(tok, "expected an identifier");
    }
    return strncopy(tok->p, tok->len);
}

static Node* parse_primary(ParserState* state);
static Node* parse_postfix(ParserState* state);
static Node* parse_unary(ParserState* state);
static Node* parse_mul(ParserState* state);
static Node* parse_add(ParserState* state);
static Node* parse_relational(ParserState* state);
static Node* parse_equality(ParserState* state);
static Node* parse_assign(ParserState* state);
static Node* parse_funccall(ParserState* state);
static Node* parse_expr(ParserState* state);
static Node* parse_expr_stmt(ParserState* state);
static Node* parse_compound_stmt(ParserState* state);
static Node* parse_decl(ParserState* state);
static Type* parse_declspec(ParserState* state);
static Type* parse_declarator(ParserState* state, Type* type);

static Node* new_add(Node* lhs, Node* rhs, Token* tok);
static Node* new_sub(Node* lhs, Node* rhs, Token* tok);

// primary = "(" expr ")" | "sizeof" unary | ident func-args? | str | num
static Node* parse_primary(ParserState* state)
{
    if (consume(state, "(")) {
        Node* node = parse_expr(state);
        expect(state, ")"); // consume ')'
        return node;
    }

    Token* tok = peek(state);
    if (consume(state, "sizeof")) {
        Node* node = parse_unary(state);
        add_type(node);
        return new_num(node->type->size, tok);
    }

    if (tok->kind == TK_NUM) {
        Node* node = new_num(tok->val, tok);
        read(state);
        return node;
    }

    if (tok->kind == TK_STR) {
        Obj* var = new_string_literal(tok->str, tok->type, tok);
        read(state);
        return new_var(var, tok);
    }

    if (tok->kind == TK_IDENT) {
        if (is_token_equal(peek_n(state, 1), "(")) {
            return parse_funccall(state);
        }

        Obj* var = find_var(tok);
        if (!var) {
            error_tok(tok, "undefined variable '%s'", tok->raw);
        }

        read(state);
        return new_var(var, tok);
    }

    error_tok(tok, "expected expression before '%s' token", tok->raw);
    return NULL;
}

// unary = ("+" | "-" | "*" | "&") unary
//       | postfix
static Node* parse_unary(ParserState* state)
{
    if (consume(state, "+")) {
        return parse_unary(state);
    }

    Token* tok = peek(state);
    if (consume(state, "-")) {
        return new_unary(ND_NEG, parse_unary(state), tok);
    }

    if (consume(state, "*")) {
        return new_unary(ND_DEREF, parse_unary(state), tok);
    }

    if (consume(state, "&")) {
        return new_unary(ND_ADDR, parse_unary(state), tok);
    }

    return parse_postfix(state);
}

// struct-members = (declspec declarator (","  declarator)* ";")*
static void parse_struct_members(ParserState* state, Type* ty)
{
    Member head = { .next = NULL };
    Member* cur = &head;

    while (!consume(state, "}")) {
        Type* basety = parse_declspec(state);
        int i = 0;
        while (!consume(state, ";")) {
            if (i++) {
                expect(state, ",");
            }

            Member* member = calloc(1, sizeof(Member));
            member->type = parse_declarator(state, basety);
            member->name = member->type->name;
            cur = cur->next = member;
        }
    }
    ty->members = head.next;
}

// struct-union-decl = ident? ("{" struct-members)?
static Type* parse_struct_union_decl(ParserState* state)
{
    // Read a struct tag.
    Token* tag = NULL;
    Token* tok = peek(state);
    if (tok->kind == TK_IDENT) {
        tag = tok;
        read(state);
    }

    if (tag && !equal(state, "{")) {
        Type* ty = find_tag(tag);
        if (!ty) {
            error_tok(tag, "unknown struct %s", tag->raw);
        }
        return ty;
    }

    // Construct a struct object.
    expect(state, "{");

    Type* ty = calloc(1, sizeof(Type));
    ty->eTypeKind = TY_INVALID;
    ty->align = 1;
    parse_struct_members(state, ty);

    if (tag) {
        push_tag_scope(tag, ty);
    }

    return ty;
}

// struct-decl = ident? "{" struct-members
static Type* parse_struct_decl(ParserState* state)
{
    Type* ty = parse_struct_union_decl(state);
    ty->eTypeKind = TY_STRUCT;

    // Assign offsets within the struct to members.
    int offset = 0;
    for (Member* mem = ty->members; mem; mem = mem->next) {
        offset = ALIGN_TO(offset, mem->type->align);
        mem->offset = offset;
        offset += mem->type->size;

        ty->align = MAX(ty->align, mem->type->align);
    }
    ty->size = ALIGN_TO(offset, ty->align);

    return ty;
}

// union-decl = struct-union-decl
static Type* parse_union_decl(ParserState* state)
{
    Type* ty = parse_struct_union_decl(state);
    ty->eTypeKind = TY_UNION;
    for (Member* mem = ty->members; mem; mem = mem->next) {
        ty->align = MAX(ty->align, mem->type->align);
        ty->size = MAX(ty->size, mem->type->size);
    }
    ty->size = ALIGN_TO(ty->size, ty->align);
    return ty;
}

static Member* get_struct_member(Type* ty, Token* tok)
{
    for (Member* mem = ty->members; mem; mem = mem->next) {
        assert(tok->raw && mem->name->raw);
        if (streq(tok->raw, mem->name->raw)) {
            return mem;
        }
    }
    error_tok(tok, "no member '%s'", tok->raw);
    return NULL;
}

static Node* struct_ref(Node* lhs, Token* tok)
{
    add_type(lhs);
    const TypeKind k = lhs->type->eTypeKind;
    if (k != TY_STRUCT && k != TY_UNION) {
        error_tok(lhs->tok, "not a struct or union");
    }

    Node* node = new_unary(ND_MEMBER, lhs, tok);
    node->member = get_struct_member(lhs->type, tok);
    return node;
}

// postfix = primary ("[" expr "]" | "." ident | "->" ident)*
static Node* parse_postfix(ParserState* state)
{
    Node* node = parse_primary(state);

    for (;;) {
        Token* tok = peek(state);
        if (consume(state, "[")) {
            // x[y] is short for *(x+y)
            Node* idx = parse_expr(state);
            expect(state, "]");
            node = new_unary(ND_DEREF, new_add(node, idx, tok), tok);
            continue;
        }

        if (consume(state, ".")) {
            node = struct_ref(node, peek(state));
            read(state);
            continue;
        }

        if (consume(state, "->")) {
            // x->y is short for (*x).y
            node = new_unary(ND_DEREF, node, tok);
            node = struct_ref(node, peek(state));
            read(state);
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

static Node* parse_binary_internal(ParserState* state, char const** symbols, ParseBinaryFn fp)
{
    Node* node = fp(state);

    for (;;) {
        bool found = false;
        for (const char** p = symbols; *p; ++p) {
            const Token* tok = peek(state);
            if (is_token_equal(tok, *p)) {
                read(state);
                NodeKind kind = to_binary_node_kind(*p);
                assert(kind != ND_INVALID);
                node = new_binary(kind, node, fp(state), tok);
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
static Node* parse_mul(ParserState* state)
{
    static char const* s_symbols[] = { "*", "/", "%", NULL };
    return parse_binary_internal(state, s_symbols, parse_unary);
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
    return NULL;
}

// add = mul ("+" mul | "-" mul)*
static Node* parse_add(ParserState* state)
{
    Node* node = parse_mul(state);
    Token* start = peek(state);

    for (;;) {
        if (consume(state, "+")) {
            node = new_add(node, parse_mul(state), start);
            continue;
        }

        if (consume(state, "-")) {
            node = new_sub(node, parse_mul(state), start);
            continue;
        }

        return node;
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node* parse_relational(ParserState* state)
{
    static char const* s_symbols[] = { "<", "<=", ">", ">=", NULL };
    return parse_binary_internal(state, s_symbols, parse_add);
}

// equality = relational ("==" relational | "!=" relational)*
static Node* parse_equality(ParserState* state)
{
    static char const* s_symbols[] = { "==", "!=", NULL };
    return parse_binary_internal(state, s_symbols, parse_relational);
}

// assign = equality ("=" assign)?
static Node* parse_assign(ParserState* state)
{
    Node* node = parse_equality(state);
    Token* tok = peek(state);
    if (is_token_equal(tok, "=")) {
        read(state);
        node = new_binary(ND_ASSIGN, node, parse_assign(state), tok);
    }
    return node;
}

// funcall = ident "(" (assign ("," assign)*)? ")"
static Node* parse_funccall(ParserState* state)
{
    const Token* funcname = read(state);
    expect(state, "(");
    Node head = { .next = NULL };
    Node* cur = &head;
    int argc = 0;
    for (; !consume(state, ")"); ++argc) {
        if (argc) {
            expect(state, ",");
        }
        cur = cur->next = parse_assign(state);
    }
    Node* node = new_node(ND_FUNCCALL, funcname);
    assert(funcname->raw);
    node->funcname = strdup(funcname->raw);
    node->args = head.next;
    node->argc = argc;
    return node;
}

// expr = assign ("," expr)?
static Node* parse_expr(ParserState* state)
{
    Node* node = parse_assign(state);

    Token* tok = peek(state);
    if (consume(state, ",")) {
        return new_binary(ND_COMMA, node, parse_expr(state), tok);
    }

    return node;
}

// expr-stmt = expr? ";"
static Node* parse_expr_stmt(ParserState* state)
{
    Token* tok = peek(state);
    if (consume(state, ";")) {
        return new_node(ND_BLOCK, tok);
    }
    Node* node = new_unary(ND_EXPR_STMT, parse_expr(state), tok);
    expect(state, ";");
    return node;
}

// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "for" "(" expr-stmt expr? ";" expr? ")" stmt
//      | "while" "(" expr ")" stmt
//      | "{" compound-stmt "}"
//      | expr-stmt
static Node* parse_stmt(ParserState* state)
{
    Token* tok = peek(state);

    if (consume(state, "return")) {
        Node* node = new_unary(ND_RETURN, parse_expr(state), tok);
        expect(state, ";");
        return node;
    }

    if (consume(state, "if")) {
        Node* node = new_node(ND_IF, tok);
        expect(state, "(");
        node->cond = parse_expr(state);
        expect(state, ")");
        node->then = parse_stmt(state);
        if (consume(state, "else")) {
            node->els = parse_stmt(state);
        }
        return node;
    }

    if (consume(state, "for")) {
        Node* node = new_node(ND_FOR, tok);

        expect(state, "(");
        node->init = parse_expr_stmt(state);
        if (!consume(state, ";")) {
            node->cond = parse_expr(state);
            expect(state, ";");
        }
        if (!consume(state, ")")) {
            node->inc = parse_expr(state);
            expect(state, ")");
        }

        node->then = parse_stmt(state);
        return node;
    }

    if (consume(state, "while")) {
        Node* node = new_node(ND_FOR, tok);
        expect(state, "(");
        node->cond = parse_expr(state);
        expect(state, ")");
        node->then = parse_stmt(state);
        return node;
    }

    if (consume(state, "{")) {
        return parse_compound_stmt(state);
    }

    return parse_expr_stmt(state);
}

static bool is_typename(Token* tok)
{
    static const char* s_types[] = {
#define DEFINE_BASE_TYPE(name, enum, sz, al) #name,
#include "base_type.inl"
#undef DEFINE_BASE_TYPE
        "struct",
        "union",
    };

    for (size_t i = 0; i < ARRAY_COUNTER(s_types); ++i) {
        if (is_token_equal(tok, s_types[i])) {
            return true;
        }
    }

    return false;
}

// compound-stmt = (declaration | stmt)* "}"
static Node* parse_compound_stmt(ParserState* state)
{
    Token* tok = peek(state); // TODO: fix token
    Node head = { .next = NULL };
    Node* cur = &head;

    enter_scope();

    while (!consume(state, "}")) {
        if (is_typename(peek(state))) {
            cur = cur->next = parse_decl(state);
        } else {
            cur = cur->next = parse_stmt(state);
        }
        add_type(cur);
    }

    leave_scope();

    Node* node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    return node;
}

// declspec = "void" | "char" | "short" | "int" | "long"
//          | struct-decl | union-decl
static Type* parse_declspec(ParserState* state)
{
    enum {
        VOID = 1 << 0,
        CHAR = 1 << 2,
        SHORT = 1 << 4,
        INT = 1 << 6,
        LONG = 1 << 8,
        OTHER = 1 << 10,
    };

    Token* tok = peek(state);
    Type* ty = g_int_type;
    int counter = 0;

    while (is_typename(peek(state))) {
        // Handle user-defined types.
        if (consume(state, "struct")) {
            ty = parse_struct_decl(state);
            counter += OTHER;
            continue;
        }

        if (consume(state, "union")) {
            ty = parse_union_decl(state);
            counter += OTHER;
            continue;
        }

        if (consume(state, "void"))
            counter += VOID;
        else if (consume(state, "char"))
            counter += CHAR;
        else if (consume(state, "short"))
            counter += SHORT;
        else if (consume(state, "int"))
            counter += INT;
        else if (consume(state, "long"))
            counter += LONG;
        else
            UNREACHABLE();

        switch (counter) {
        case VOID:
            ty = g_void_type;
            break;
        case CHAR:
            ty = g_char_type;
            break;
        case SHORT:
        case SHORT + INT:
            ty = g_short_type;
            break;
        case INT:
            ty = g_int_type;
            break;
        case LONG:
        case LONG + INT:
        case LONG + LONG:
        case LONG + LONG + INT:
            ty = g_long_type;
            break;
        default:
            error_tok(tok, "invalid type specifer");
        }
    }

    return ty;
}

// type-suffix = ("(" func-params? ")")?
// func-params = param ("," param)*
// param       = declspec declarator
static Type* parse_func_params(ParserState* state, Type* type)
{
    Type head = { .next = NULL };
    Type* cur = &head;
    while (!consume(state, ")")) {
        if (cur != &head) {
            expect(state, ",");
        }
        Type* basety = parse_declspec(state);
        Type* ty = parse_declarator(state, basety);
        cur = cur->next = copy_type(ty);
    }
    type = func_type(type);
    type->params = head.next;
    return type;
}

static int64_t get_number(Token* tok)
{
    if (tok->kind != TK_NUM) {
        error_tok(tok, "expected a number");
    }
    return tok->val;
}

// type-suffix = "(" func-params
//             | "[" num "]" type-suffix
//             | ε
static Type* parse_type_suffix(ParserState* state, Type* type)
{
    if (consume(state, "(")) {
        return parse_func_params(state, type);
    }

    if (consume(state, "[")) {
        int arrayLen = (int)get_number(peek(state));
        read(state);
        expect(state, "]");
        type = parse_type_suffix(state, type);
        return array_of(type, arrayLen);
    }

    return type;
}

// declarator = "*"* ident type-suffix
static Type* parse_declarator(ParserState* state, Type* type)
{
    while (consume(state, "*")) {
        type = pointer_to(type);
    }

    Token* tok = peek(state);
    if (tok->kind != TK_IDENT) {
        error_tok(tok, "expected a variable name");
    }

    read(state);
    type = parse_type_suffix(state, type);
    type->name = tok;
    return type;
}

// declaration = declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
static Node* parse_decl(ParserState* state)
{
    Type* base_type = parse_declspec(state);
    Node head = { .next = NULL };
    Node* cur = &head;
    int i = 0;
    while (!consume(state, ";")) {
        if (i++ > 0) {
            expect(state, ",");
        }

        Type* type = parse_declarator(state, base_type);
        Obj* var = new_lvar(get_ident(type->name), type);

        const Token* tok = peek(state);
        if (is_token_equal(tok, "=")) {
            read(state);
            Node* lhs = new_var(var, type->name);
            Node* rhs = parse_assign(state);
            Node* node = new_binary(ND_ASSIGN, lhs, rhs, tok);
            cur = cur->next = new_unary(ND_EXPR_STMT, node, lhs->tok);
        }
    }

    Node* node = new_node(ND_BLOCK, peek(state));
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

static void parse_global_variable(ParserState* state, Type* basety)
{
    int i = 0;
    while (!consume(state, ";")) {
        if (i) {
            expect(state, ",");
        }
        ++i;

        Type* type = parse_declarator(state, basety);
        new_gvar(get_ident(type->name), type);
    }
}

static Obj* parse_function(ParserState* state, Type* basetpye)
{
    Type* type = parse_declarator(state, basetpye);
    Obj* fn = new_gvar(get_ident(type->name), type);
    fn->isFunc = true;
    fn->isDefinition = !consume(state, ";");
    if (!fn->isDefinition) {
        return fn;
    }

    s_locals = NULL;

    enter_scope();

    create_param_lvars(type->params);
    fn->params = s_locals;
    expect(state, "{");
    fn->body = parse_compound_stmt(state);
    fn->locals = s_locals;

    leave_scope();

    return fn;
}

// Lookahead tokens and returns true if a given token is a start
// of a function definition or declaration.
static bool is_function(ParserState* state)
{
    if (equal(state, ";")) {
        return false;
    }

    Type dummy;
    Type* type = parse_declarator(state, &dummy);
    return type->eTypeKind == TY_FUNC;
}

// program = (function-definition | global-variable)*
Obj* parse(List* tokens)
{
    ParserState state;
    state.reader.tokens = tokens;
    state.reader.cursor = tokens->front;

    s_globals = NULL;
    while (peek(&state)->kind != TK_EOF) {
        Type* basetype = parse_declspec(&state);

        // restore index
        // because we only need to peek ahead to find out if object is a function or not
        ListNode* oldCursor = state.reader.cursor;
        bool isFunc = is_function(&state);
        state.reader.cursor = oldCursor;
        if (isFunc) {
            parse_function(&state, basetype);
        } else {
            parse_global_variable(&state, basetype);
        }
    }
    return s_globals;
}
