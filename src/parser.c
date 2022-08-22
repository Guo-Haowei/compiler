#include "minic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    bool isTypedef;
} VarAttrib;

// Scope for struct or union tags
typedef struct {
    char* name;
    Type* ty;
} TagScope;

// Scope for local or global variables.
typedef struct {
    char* name;
    Obj* var;
    Type* typeDef;
} VarScope;

// Represents a block scope.
typedef struct Scope {
    List vars;
    List tags;
} Scope;

typedef struct {
    TokenReader reader;
    List scopes;
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

static void enter_scope(ParserState* state)
{
    Scope scope;
    memset(&scope, 0, sizeof(Scope));
    _list_push_back(&(state->scopes), &scope, sizeof(Scope));
}

static void leave_scope(ParserState* state)
{
    assert(state->scopes.len);
    list_pop_back(&(state->scopes));
}

static VarScope* push_var_scope(ParserState* state, char* name)
{
    Scope* scope = list_back(Scope, &(state->scopes));
    VarScope sc;
    ZERO_MEMORY(sc);
    sc.name = name;
    list_push_back(&(scope->vars), sc);
    return _list_back(&(scope->vars));
}

static TagScope* push_tag_scope(ParserState* state, Token* tok, Type* ty)
{
    Scope* scope = list_back(Scope, &(state->scopes));
    TagScope sc;
    ZERO_MEMORY(sc);
    sc.name = strncopy(tok->p, tok->len);
    sc.ty = ty;
    list_push_back(&(scope->tags), sc);
    return _list_back(&(scope->tags));
}

/**
 * Create Node API
 */
static Node* new_node(NodeKind eNodeKind, Token* tok)
{
    static int s_id = 0;
    Node* node = calloc(1, sizeof(Node));
    node->id = s_id++;
    node->tok = tok;
    node->eNodeKind = eNodeKind;
    return node;
}

static Node* new_num(int64_t val, Token* tok)
{
    Node* node = new_node(ND_NUM, tok);
    node->eNodeKind = ND_NUM;
    node->val = val;
    return node;
}

static Node* new_var(Obj* var, Token* tok)
{
    Node* node = new_node(ND_VAR, tok);
    node->var = var;
    return node;
}

static Node* new_binary(NodeKind eNodeKind, Node* lhs, Node* rhs, Token* tok)
{
    Node* node = new_node(eNodeKind, tok);
    node->isBinary = true;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node* new_unary(NodeKind eNodeKind, Node* expr, Token* tok)
{
    Node* node = new_node(eNodeKind, tok);
    node->isUnary = true;
    node->lhs = expr;
    return node;
}

Node* new_cast(Node* expr, Type* type, Token* tok)
{
    add_type(expr);
    Node* node = new_node(ND_CAST, expr->tok);
    node->tok = tok;
    node->lhs = expr;
    node->type = copy_type(type);
    return node;
}

static Obj* new_variable(ParserState* state, char* name, Type* type)
{
    static int s_id = 0;
    Obj* var = calloc(1, sizeof(Obj));
    var->id = s_id++;
    var->name = name;
    var->type = type;
    push_var_scope(state, name)->var = var;
    return var;
}

static Obj* new_lvar(ParserState* state, char* name, Type* type)
{
    Obj* var = new_variable(state, name, type);
    var->isLocal = true;
    var->next = s_locals;
    s_locals = var;
    return var;
}

static Obj* new_gvar(ParserState* state, char* name, Type* type)
{
    Obj* var = new_variable(state, name, type);
    var->next = s_globals;
    s_globals = var;
    return var;
}

typedef Node* (*ParseBinaryFn)(ParserState*);

// Find a local variable by name.
static Type* find_tag(ParserState* state, const Token* tok)
{
    for (ListNode* s = state->scopes.back; s; s = s->prev) {
        Scope* scope = list_node_get(Scope, s);
        for (ListNode* t = scope->tags.back; t; t = t->prev) {
            TagScope* tag = list_node_get(TagScope, t);
            if (streq(tok->raw, tag->name)) {
                return tag->ty;
            }
        }
    }
    return NULL;
}

static VarScope* find_var(ParserState* state, const Token* tok)
{
    for (ListNode* s = state->scopes.back; s; s = s->prev) {
        Scope* scope = list_node_get(Scope, s);
        for (ListNode* t = scope->vars.back; t; t = t->prev) {
            VarScope* var = list_node_get(VarScope, t);
            if (streq(tok->raw, var->name)) {
                return var;
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

static Obj* new_anon_gvar(ParserState* state, Type* ty)
{
    return new_gvar(state, new_unique_name(), ty);
}

static Obj* new_string_literal(ParserState* state, char* p, Type* type, Token* tok)
{
    Obj* var = new_anon_gvar(state, type);
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
static Node* parse_cast(ParserState* state);
static Node* parse_mul(ParserState* state);
static Node* parse_add(ParserState* state);
static Node* parse_relational(ParserState* state);
static Node* parse_equality(ParserState* state);
static Node* parse_assign(ParserState* state);
static Node* parse_funccall(ParserState* state);
static Node* parse_expr(ParserState* state);
static Node* parse_expr_stmt(ParserState* state);
static Node* parse_compound_stmt(ParserState* state);
static Node* parse_decl(ParserState* state, Type* baseType);
static Type* parse_declspec(ParserState* state, VarAttrib* attrib);
static Type* parse_declarator(ParserState* state, Type* type);
static void parse_typedef(ParserState* state, Type* baseType);
static bool is_type_name(ParserState* state, Token* tok);
static Type* parse_type_name(ParserState* state);
static Type* parse_type_suffix(ParserState* state, Type* type);

static Node* new_add(Node* lhs, Node* rhs, Token* tok);
static Node* new_sub(Node* lhs, Node* rhs, Token* tok);

// primary = "(" expr ")"
//         | "sizeof" "(" type-name ")"
//         | "sizeof" unary
//         | ident func-args?
//         | str
//         | num
static Node* parse_primary(ParserState* state)
{
    if (consume(state, "(")) {
        Node* node = parse_expr(state);
        expect(state, ")"); // consume ')'
        return node;
    }

    Token* tok = peek(state);
    if (consume(state, "sizeof")) {
        Token* name = peek_n(state, 1);
        if (equal(state, "(") && is_type_name(state, name)) {
            expect(state, "(");
            Type* type = parse_type_name(state);
            expect(state, ")");
            return new_num(type->size, tok);
        }

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
        Obj* var = new_string_literal(state, tok->str, tok->type, tok);
        read(state);
        return new_var(var, tok);
    }

    if (tok->kind == TK_IDENT) {
        if (is_token_equal(peek_n(state, 1), "(")) {
            return parse_funccall(state);
        }

        VarScope* sc = find_var(state, tok);
        if (!sc || !sc->var) {
            error_tok(tok, "undefined variable '%s'", tok->raw);
        }

        read(state);
        return new_var(sc->var, tok);
    }

    error_tok(tok, "expected expression before '%s' token", tok->raw);
    return NULL;
}

// unary = ("+" | "-" | "*" | "&") cast
//       | postfix
static Node* parse_unary(ParserState* state)
{
    if (consume(state, "+")) {
        return parse_cast(state);
    }

    Token* tok = peek(state);
    if (consume(state, "-")) {
        return new_unary(ND_NEG, parse_cast(state), tok);
    }

    if (consume(state, "*")) {
        return new_unary(ND_DEREF, parse_cast(state), tok);
    }

    if (consume(state, "&")) {
        return new_unary(ND_ADDR, parse_cast(state), tok);
    }

    return parse_postfix(state);
}

// struct-members = (declspec declarator (","  declarator)* ";")*
static void parse_struct_members(ParserState* state, Type* ty)
{
    Member head = { .next = NULL };
    Member* cur = &head;

    while (!consume(state, "}")) {
        Type* basety = parse_declspec(state, NULL);
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
        Type* ty = find_tag(state, tag);
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
        push_tag_scope(state, tag, ty);
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

// cast = "(" type-name ")" cast | unary
static Node* parse_cast(ParserState* state)
{
    if (equal(state, "(") && is_type_name(state, peek_n(state, 1))) {
        Token* start = read(state);
        Type* type = parse_type_name(state);
        expect(state, ")");
        Node* node = new_cast(parse_cast(state), type, start);
        return node;
    }
    return parse_unary(state);
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

static Node* parse_binary_internal(ParserState* state, const char** symbols, ParseBinaryFn fp)
{
    Node* node = fp(state);

    for (;;) {
        bool found = false;
        for (const char** p = symbols; *p; ++p) {
            Token* tok = peek(state);
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

// mul = cast ("*" cast | "/" cast)*
static Node* parse_mul(ParserState* state)
{
    static char const* s_symbols[] = { "*", "/", "%", NULL };
    return parse_binary_internal(state, s_symbols, parse_cast);
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
    Token* funcname = read(state);
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

static Type* find_typedef(ParserState* state, Token* tok)
{
    if (tok->kind == TK_IDENT) {
        VarScope* sc = find_var(state, tok);
        if (sc) {
            return sc->typeDef;
        }
    }
    return NULL;
}

static bool is_type_name(ParserState* state, Token* tok)
{
    // clang-format off
    static const char* kw[] = {
        "char", "int", "long", "short", "struct", "typedef", "union", "void", NULL,
    };
    // clang-format on

    for (const char** p = kw; *p; ++p) {
        if (is_token_equal(tok, *p)) {
            return true;
        }
    }

    return find_typedef(state, tok);
}

// compound-stmt = (typedef | declaration | stmt)* "}"
static Node* parse_compound_stmt(ParserState* state)
{
    Token* compoundTok = peek_n(state, -1);
    Node head = { .next = NULL };
    Node* cur = &head;

    enter_scope(state);

    for (;;) {
        Token* tok = peek(state);

        if (is_token_equal(tok, "}")) {
            read(state);
            break;
        }

        if (is_type_name(state, tok)) {
            VarAttrib attrib;
            ZERO_MEMORY(attrib);
            Type* baseType = parse_declspec(state, &attrib);

            if (attrib.isTypedef) {
                parse_typedef(state, baseType);
                continue;
            }
            cur = cur->next = parse_decl(state, baseType);
        } else {
            cur = cur->next = parse_stmt(state);
        }
        add_type(cur);
    }

    leave_scope(state);

    Node* node = new_node(ND_BLOCK, compoundTok);
    node->body = head.next;
    return node;
}

// declspec = ("void" | "char" | "short" | "int" | "long"
//             | "typedef"
//             | struct-decl | union-decl | typedef-name)+
//
// The order of typenames in a type-specifier doesn't matter. For
// example, `int long static` means the same as `static long int`.
// That can also be written as `static long` because you can omit
// `int` if `long` or `short` are specified. However, something like
// `char int` is not a valid type specifier. We have to accept only a
// limited combinations of the typenames.
//
// In this function, we count the number of occurrences of each typename
// while keeping the "current" type object that the typenames up
// until that point represent. When we reach a non-typename token,
// we returns the current type object.
static Type* parse_declspec(ParserState* state, VarAttrib* attrib)
{
    enum {
        VOID = 1 << 0,
        CHAR = 1 << 2,
        SHORT = 1 << 4,
        INT = 1 << 6,
        LONG = 1 << 8,
        OTHER = 1 << 10,
    };

    Token* tok = NULL;
    Type* ty = g_int_type;
    int counter = 0;

    for (;;) {
        tok = peek(state);
        if (!is_type_name(state, tok)) {
            break;
        }

        // "typedef"
        if (consume(state, "typedef")) {
            if (!attrib) {
                error_tok(tok, "storage class specifier is not allowed in this context");
            }
            attrib->isTypedef = true;
            continue;
        }

        // user defined types
        Type* ty2 = find_typedef(state, tok);
        const bool isStruct = is_token_equal(tok, "struct");
        const bool isUnion = is_token_equal(tok, "union");
        if (isStruct || isUnion || ty2) {
            if (counter) {
                break;
            }

            read(state);
            if (isStruct) {
                ty = parse_struct_decl(state);
            } else if (isUnion) {
                ty = parse_union_decl(state);
            } else {
                ty = ty2;
            }

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

// abstract-declarator = "*"* ("(" abstract-declarator ")")? type-suffix
static Type* parse_abstract_declarator(ParserState* state, Type* ty)
{
    while (consume(state, "*")) {
        ty = pointer_to(ty);
    }

    if (equal(state, "(")) {
        assert(0 && "This is not supported!");
    }

    return parse_type_suffix(state, ty);
}

// type-name = declspec abstract-declarator
static Type* parse_type_name(ParserState* state)
{
    Type* type = parse_declspec(state, NULL);
    return parse_abstract_declarator(state, type);
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
        Type* basety = parse_declspec(state, NULL);
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
static Node* parse_decl(ParserState* state, Type* baseType)
{
    Node head = { .next = NULL };
    Node* cur = &head;
    int i = 0;
    while (!consume(state, ";")) {
        if (i++ > 0) {
            expect(state, ",");
        }

        Type* type = parse_declarator(state, baseType);
        Obj* var = new_lvar(state, get_ident(type->name), type);

        Token* tok = peek(state);
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

static void parse_typedef(ParserState* state, Type* baseType)
{
    bool first = true;
    while (!consume(state, ";")) {
        if (!first) {
            expect(state, ",");
        }
        first = false;
        Type* ty = parse_declarator(state, baseType);
        push_var_scope(state, get_ident(ty->name))->typeDef = ty;
    }
}

// @TODO: refactor to use list
static void create_param_lvars(ParserState* state, Type* param)
{
    if (param) {
        create_param_lvars(state, param->next);
        new_lvar(state, get_ident(param->name), param);
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
        new_gvar(state, get_ident(type->name), type);
    }
}

static Obj* parse_function(ParserState* state, Type* basetpye)
{
    Type* type = parse_declarator(state, basetpye);
    Obj* fn = new_gvar(state, get_ident(type->name), type);
    fn->isFunc = true;
    fn->isDefinition = !consume(state, ";");
    if (!fn->isDefinition) {
        return fn;
    }

    s_locals = NULL;

    enter_scope(state);

    create_param_lvars(state, type->params);
    fn->params = s_locals;
    expect(state, "{");
    fn->body = parse_compound_stmt(state);
    fn->locals = s_locals;

    leave_scope(state);

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

// program = (typedef | function-definition | global-variable)*
Obj* parse(List* tokens)
{
    ParserState state;
    memset(&state, 0, sizeof(ParserState));
    state.reader.tokens = tokens;
    state.reader.cursor = tokens->front;

    enter_scope(&state);

    s_globals = NULL;
    while (peek(&state)->kind != TK_EOF) {
        VarAttrib attrib;
        ZERO_MEMORY(attrib);

        Type* baseType = parse_declspec(&state, &attrib);
        if (attrib.isTypedef) {
            parse_typedef(&state, baseType);
            continue;
        }

        // restore index
        // because we only need to peek ahead to find out if object is a function or not
        ListNode* oldCursor = state.reader.cursor;
        bool isFunc = is_function(&state);
        state.reader.cursor = oldCursor;
        if (isFunc) {
            parse_function(&state, baseType);
        } else {
            parse_global_variable(&state, baseType);
        }
    }

    leave_scope(&state);
    assert(state.scopes.len == 0);

    return s_globals;
}
