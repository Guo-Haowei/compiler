#include "cc.h"

typedef struct {
    bool isTypedef;
    bool isStatic;
    bool isExtern;
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
    Type* enumType;
    int enumVal;
} VarScope;

// Represents a block scope.
typedef struct Scope {
    // C has two block scopes; one is for variables/typedefs and
    // the other is for struct/union/enum tags.
    Dict* vars;
    Dict* tags;
} Scope;

typedef struct {
    TokenReader reader;
    List scopes;
    Obj* currentFunc;

    // lists of all goto statements and labels in the curent function.
    Node* gotos;
    Node* labels;

    char* brkLabel;
    char* cntLabel;

    Node* currentSwitch;
    Obj* locals;
    Obj* globals;
} ParserState;

// This struct represents a variable initializer. Since initializers
// can be nested (e.g. `int x[2][2] = {{1, 2}, {3, 4}}`), this struct
// is a tree data structure.
typedef struct Initializer Initializer;
struct Initializer {
    Initializer* next;
    Type* type;
    Token* tok;
    // If it's not an aggregate type and has an initializer,
    // `expr` has an initialization expression.
    Node* expr;
    // If it's an initializer for an aggregate type (e.g. array or struct),
    // `children` has initializers for its children.
    Initializer** children;
};

// For local variable initializer.
typedef struct InitDesg InitDesg;
struct InitDesg {
    InitDesg* next;
    int idx;
    Member* member;
    Obj* var;
};

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

static bool equal(ParserState* state, char* symbol)
{
    return tr_equal(&(state->reader), symbol);
}

static bool consume(ParserState* state, char* symbol)
{
    return tr_consume(&(state->reader), symbol);
}

static void expect(ParserState* state, char* symbol)
{
    tr_expect(&(state->reader), symbol);
}

// All local variable instances created during parsing are
// accumulated to this list.

static void enter_scope(ParserState* state)
{
    Scope scope;
    memset(&scope, 0, sizeof(Scope));
    scope.vars = dict_new();
    scope.tags = dict_new();
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
    VarScope* sc = calloc(1, sizeof(VarScope));
    sc->name = name;
    dict_try_add(scope->vars, name, sc);
    return sc;
}

static TagScope* push_tag_scope(ParserState* state, Token* tok, Type* ty)
{
    Scope* scope = list_back(Scope, &(state->scopes));
    TagScope* sc = calloc(1, sizeof(TagScope));
    sc->name = tok->raw;
    sc->ty = ty;
    dict_try_add(scope->tags, tok->raw, sc);
    return sc;
}

/**
 * Create Node API
 */
static Node* new_node(NodeKind kind, Token* tok)
{
    Node* node = calloc(1, ALIGN(sizeof(Node), 16));
    node->tok = tok;
    node->kind = kind;
    return node;
}

static Node* new_num(int64_t val, Token* tok)
{
    Node* node = new_node(ND_NUM, tok);
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

static Node* new_ulong(int64_t val, Token* tok)
{
    Node* node = new_num(val, tok);
    node->type = &g_ulong_type;
    return node;
}

static Node* new_var(Obj* var, Token* tok)
{
    Node* node = new_node(ND_VAR, tok);
    node->var = var;
    return node;
}

static Node* new_binary(NodeKind kind, Node* lhs, Node* rhs, Token* tok)
{
    Node* node = new_node(kind, tok);
    node->isBinary = true;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node* new_unary(NodeKind kind, Node* expr, Token* tok)
{
    Node* node = new_node(kind, tok);
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
    Obj* var = calloc(1, ALIGN(sizeof(Obj), 16));
    var->id = unique_id();
    var->name = name;
    var->type = type;
    push_var_scope(state, name)->var = var;
    return var;
}

static Obj* new_lvar(ParserState* state, char* name, Type* type)
{
    Obj* var = new_variable(state, name, type);
    var->isLocal = true;
    var->next = state->locals;
    state->locals = var;
    return var;
}

static Obj* new_gvar(ParserState* state, char* name, Type* type)
{
    Obj* var = new_variable(state, name, type);
    var->next = state->globals;
    var->isDefinition = true;
    state->globals = var;
    return var;
}

// Find a local variable by name.
static Type* find_tag(ParserState* state, Token* tok)
{
    for (ListNode* s = state->scopes.back; s; s = s->prev) {
        Scope* scope = list_node_get(Scope, s);
        TagScope* tag = dict_get(scope->tags, tok->raw);
        if (tag) {
            return tag->ty;
        }
    }
    return NULL;
}

static VarScope* find_var(ParserState* state, Token* tok)
{
    for (ListNode* s = state->scopes.back; s; s = s->prev) {
        Scope* scope = list_node_get(Scope, s);
        VarScope* var = dict_get(scope->vars, tok->raw);
        if (var) {
            return var;
        }
    }
    return NULL;
}

static char* new_unique_name()
{
    char buf[128];
    snprintf(buf, sizeof(buf), ".L.anon.%d", unique_id());
    return strdup(buf);
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

static char* get_ident(Token* tok)
{
    if (tok->kind != TK_IDENT) {
        error_tok(tok, "expected an identifier");
    }
    return (tok->raw);
}

static Type* parse_enum_specifier(ParserState* state);
static Node* parse_primary(ParserState* state);
static Node* parse_postfix(ParserState* state);
static Node* parse_unary(ParserState* state);
static Node* parse_cast(ParserState* state);
static Node* parse_mul(ParserState* state);
static Node* parse_add(ParserState* state);
static Node* parse_shift(ParserState* state);
static Node* parse_relational(ParserState* state);
static Node* parse_equality(ParserState* state);
static Node* parse_bitor(ParserState* state);
static Node* parse_bitxor(ParserState* state);
static Node* parse_bitand(ParserState* state);
static Node* parse_logor(ParserState* state);
static Node* parse_logand(ParserState* state);
static Node* parse_ternary(ParserState* state);
static Node* parse_assign(ParserState* state);
static Node* parse_funccall(ParserState* state);
static Node* parse_expr(ParserState* state);
static Node* parse_expr_stmt(ParserState* state);
static Node* parse_compound_stmt(ParserState* state);
static Node* parse_declaration(ParserState* state, Type* baseType, VarAttrib* attrib);
static Type* parse_declspec(ParserState* state, VarAttrib* attrib);
static Type* parse_declarator(ParserState* state, Type* type);
static void parse_gvar_initializer(ParserState* state, Obj* var);
static Node* parse_lvar_initializer(ParserState* state, Obj* var);
static void parse_typedef(ParserState* state, Type* baseType);
static bool is_type_name(ParserState* state, Token* tok);
static Type* parse_type_name(ParserState* state);
static Type* parse_type_suffix(ParserState* state, Type* type);
static int64_t parse_constexpr(ParserState* state);
static Initializer* parse_initializer(ParserState* state, Type* ty);

static Node* new_add(Node* lhs, Node* rhs, Token* tok);
static Node* new_sub(Node* lhs, Node* rhs, Token* tok);
static Node* to_assign(ParserState* state, Node* binary);

static Initializer* new_initializer(Type* ty, bool flex);
static void initializer2(ParserState* state, Initializer* init);

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
            return new_ulong(type->size, tok);
        }

        Node* node = parse_unary(state);
        add_type(node);
        return new_ulong(node->type->size, tok);
    }

    if (consume(state, "__builtin_reg_class")) {
        expect(state, "(");
        Type* ty = parse_type_name(state);
        expect(state, ")");
        if (is_integer(ty) || ty->kind == TY_PTR) {
            return new_num(0, tok);
        }
        return new_num(2, tok);
    }

    if (tok->kind == TK_NUM) {
        Node* node = new_num(tok->val, tok);
        node->type = tok->type;
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
        if (!sc || !(sc->var || sc->enumType)) {
            error_tok(tok, "undefined variable '%s'", tok->raw);
        }

        read(state);
        return sc->var ? new_var(sc->var, tok) : new_num(sc->enumVal, tok);
    }

    error_tok(tok, "expected expression before '%s' token", tok->raw);
    return NULL;
}

// unary = ("+" | "-" | "*" | "&" | "!" | "~") cast
//       | ("++" | "--") unary
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

    if (consume(state, "!")) {
        return new_unary(ND_NOT, parse_cast(state), tok);
    }

    if (consume(state, "~")) {
        return new_unary(ND_BITNOT, parse_cast(state), tok);
    }

    // ++i => i+=1
    if (consume(state, "++")) {
        return to_assign(state, new_add(parse_unary(state), new_num(1, tok), tok));
    }

    // --i => i-=1
    if (consume(state, "--")) {
        return to_assign(state, new_sub(parse_unary(state), new_num(1, tok), tok));
    }

    return parse_postfix(state);
}

// struct-members = (declspec declarator (","  declarator)* ";")*
static void parse_struct_members(ParserState* state, Type* ty)
{
    Member head;
    ZERO_MEMORY(head);
    Member* cur = &head;
    int idx = 0;

    while (!consume(state, "}")) {
        Type* basety = parse_declspec(state, NULL);
        bool first = true;
        while (!consume(state, ";")) {
            if (!first) {
                expect(state, ",");
            }
            first = false;

            Member* mem = calloc(1, ALIGN(sizeof(Member), 16));
            mem->type = parse_declarator(state, basety);
            mem->name = mem->type->name;
            mem->idx = idx++;
            cur = cur->next = mem;
        }
    }
    ty->members = head.next;
}

// struct-union-decl = ident? ("{" struct-members)?
static Type* parse_struct_union_decl(ParserState* state)
{
    Type* ty = struct_type();

    // Read a struct tag.
    Token* tag = NULL;
    Token* tok = peek(state);
    if (tok->kind == TK_IDENT) {
        tag = tok;
        read(state);
    }

    if (tag && !equal(state, "{")) {
        Type* ty2 = find_tag(state, tag);
        if (ty2) {
            return ty2;
        }

        ty->size = -1;
        push_tag_scope(state, tag, ty);
        return ty;
    }

    // Construct a struct object.
    expect(state, "{");

    parse_struct_members(state, ty);

    if (tag) {
        Type* ty2 = find_tag(state, tag);
        if (ty2) {
            *ty2 = *ty;
            return ty2;
        }
        push_tag_scope(state, tag, ty);
    }

    return ty;
}

// struct-decl = ident? "{" struct-members
static Type* parse_struct_decl(ParserState* state)
{
    Type* ty = parse_struct_union_decl(state);
    ty->kind = TY_STRUCT;

    // Assign offsets within the struct to members.
    int offset = 0;
    for (Member* mem = ty->members; mem; mem = mem->next) {
        offset = ALIGN(offset, mem->type->align);
        mem->offset = offset;
        offset += mem->type->size;

        ty->align = MAX(ty->align, mem->type->align);
    }
    ty->size = ALIGN(offset, ty->align);

    return ty;
}

// union-decl = struct-union-decl
static Type* parse_union_decl(ParserState* state)
{
    Type* ty = parse_struct_union_decl(state);
    ty->kind = TY_UNION;
    for (Member* mem = ty->members; mem; mem = mem->next) {
        ty->align = MAX(ty->align, mem->type->align);
        ty->size = MAX(ty->size, mem->type->size);
    }
    ty->size = ALIGN(ty->size, ty->align);
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
    TypeKind k = lhs->type->kind;
    if (k != TY_STRUCT && k != TY_UNION) {
        error_tok(lhs->tok, "not a struct or union");
    }

    Node* node = new_unary(ND_MEMBER, lhs, tok);
    node->member = get_struct_member(lhs->type, tok);
    return node;
}

// Convert A++ to `(typeof A)((A += 1) - 1)`
static Node* new_inc(ParserState* state, Node* node, Token* tok, int addend)
{
    add_type(node);
    Node* inc = new_add(node, new_num(addend, tok), tok);
    Node* dec = new_num(-addend, tok);
    return new_cast(new_add(to_assign(state, inc), dec, tok), node->type, tok);
}

// postfix = primary ("[" expr "]" | "." ident | "->" ident | "++" | "--")*
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

        if (consume(state, "++")) {
            node = new_inc(state, node, tok, 1);
            continue;
        }

        if (consume(state, "--")) {
            node = new_inc(state, node, tok, -1);
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

// mul = cast ("*" cast | "/" cast | "%" cast)*
static Node* parse_mul(ParserState* state)
{
    Node* node = parse_cast(state);
    for (;;) {
        Token* tok = peek(state);
        if (is_token_equal(tok, "*")) {
            read(state);
            node = new_binary(ND_MUL, node, parse_cast(state), tok);
            continue;
        }
        if (is_token_equal(tok, "/")) {
            read(state);
            node = new_binary(ND_DIV, node, parse_cast(state), tok);
            continue;
        }
        if (is_token_equal(tok, "%")) {
            read(state);
            node = new_binary(ND_MOD, node, parse_cast(state), tok);
            continue;
        }

        return node;
    }
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
        return new_binary(ND_SUB, lhs, rhs, tok);
    }

    // ptr - ptr, which returns how many elements are between the two.
    if (lhs->type->base && rhs->type->base) {
        Node* node = new_binary(ND_SUB, lhs, rhs, tok);
        node->type = &g_long_type;
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

// shift = add ("<<" add | ">>" add)*
static Node* parse_shift(ParserState* state)
{
    Node* node = parse_add(state);
    for (;;) {
        Token* tok = peek(state);
        if (is_token_equal(tok, ">>")) {
            read(state);
            node = new_binary(ND_SHR, node, parse_add(state), tok);
            continue;
        }
        if (is_token_equal(tok, "<<")) {
            read(state);
            node = new_binary(ND_SHL, node, parse_add(state), tok);
            continue;
        }

        return node;
    }
}

// relational = shift ("<" shift | "<=" shift | ">" shift | ">=" shift)*
static Node* parse_relational(ParserState* state)
{
    Node* node = parse_shift(state);
    for (;;) {
        Token* tok = peek(state);
        if (is_token_equal(tok, "<")) {
            read(state);
            node = new_binary(ND_LT, node, parse_shift(state), tok);
            continue;
        }
        if (is_token_equal(tok, "<=")) {
            read(state);
            node = new_binary(ND_LE, node, parse_shift(state), tok);
            continue;
        }
        if (is_token_equal(tok, ">")) {
            read(state);
            node = new_binary(ND_LT, parse_shift(state), node, tok);
            continue;
        }
        if (is_token_equal(tok, ">=")) {
            read(state);
            node = new_binary(ND_LE, parse_shift(state), node, tok);
            continue;
        }
        return node;
    }
}

// equality = relational ("==" relational | "!=" relational)*
static Node* parse_equality(ParserState* state)
{
    Node* node = parse_relational(state);
    for (;;) {
        Token* tok = peek(state);
        if (is_token_equal(tok, "==")) {
            read(state);
            node = new_binary(ND_EQ, node, parse_relational(state), tok);
            continue;
        }
        if (is_token_equal(tok, "!=")) {
            read(state);
            node = new_binary(ND_NE, node, parse_relational(state), tok);
            continue;
        }
        return node;
    }
}

// Convert `A op= B` to `tmp = &A, *tmp = *tmp op B`
// where tmp is a fresh pointer variable.
static Node* to_assign(ParserState* state, Node* binary)
{
    static int s_counter;
    char uniqueName[32];
    snprintf(uniqueName, sizeof(uniqueName), "@to_assign%d", s_counter++);

    add_type(binary->lhs);
    add_type(binary->rhs);
    Token* tok = binary->tok;
    Obj* tmp = new_lvar(state, strdup(uniqueName), pointer_to(binary->lhs->type));
    Node* expr1 = new_binary(ND_ASSIGN, new_var(tmp, tok), new_unary(ND_ADDR, binary->lhs, tok), tok);
    Node* derefTmp = new_unary(ND_DEREF, new_var(tmp, tok), tok);
    Node* op = new_binary(binary->kind, new_unary(ND_DEREF, new_var(tmp, tok), tok), binary->rhs, tok);
    Node* expr2 = new_binary(ND_ASSIGN, derefTmp, op, tok);
    return new_binary(ND_COMMA, expr1, expr2, tok);
}

// bitor = bitxor ("|" bitxor)*
static Node* parse_bitor(ParserState* state)
{
    Node* node = parse_bitxor(state);
    while (equal(state, "|")) {
        Token* start = read(state);
        node = new_binary(ND_BITOR, node, parse_bitxor(state), start);
    }
    return node;
}

// bitxor = bitand ("^" bitand)*
static Node* parse_bitxor(ParserState* state)
{
    Node* node = parse_bitand(state);
    while (equal(state, "^")) {
        Token* start = read(state);
        node = new_binary(ND_BITXOR, node, parse_bitand(state), start);
    }
    return node;
}

// bitand = equality ("&" equality)*
static Node* parse_bitand(ParserState* state)
{
    Node* node = parse_equality(state);
    while (equal(state, "&")) {
        Token* start = read(state);
        node = new_binary(ND_BITAND, node, parse_equality(state), start);
    }
    return node;
}

// logand = bitor ("&&" bitor)*
static Node* parse_logand(ParserState* state)
{
    Node* node = parse_bitor(state);
    while (equal(state, "&&")) {
        Token* start = read(state);
        node = new_binary(ND_LOGAND, node, parse_bitor(state), start);
    }
    return node;
}

// logor = logand ("||" logand)*
static Node* parse_logor(ParserState* state)
{
    Node* node = parse_logand(state);
    while (equal(state, "||")) {
        Token* start = read(state);
        node = new_binary(ND_LOGOR, node, parse_logand(state), start);
    }
    return node;
}

// ternary = logor ("?" expr ":" conditional)?
static Node* parse_ternary(ParserState* state)
{
    Node* cond = parse_logor(state);
    if (!consume(state, "?")) {
        return cond;
    }

    Node* node = new_node(ND_TERNARY, cond->tok);
    node->cond = cond;
    node->then = parse_expr(state);
    expect(state, ":");
    node->els = parse_ternary(state);
    return node;
}

// assign    = ternary (assign-op assign)?
// assign-op = "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^="
//           | "<<=" | ">>="
static Node* parse_assign(ParserState* state)
{
    Node* node = parse_ternary(state);
    Token* tok = peek(state);
    if (consume(state, "=")) {
        node = new_binary(ND_ASSIGN, node, parse_assign(state), tok);
    }
    if (consume(state, "+=")) {
        return to_assign(state, new_add(node, parse_assign(state), tok));
    }
    if (consume(state, "-=")) {
        return to_assign(state, new_sub(node, parse_assign(state), tok));
    }
    if (consume(state, "*=")) {
        return to_assign(state, new_binary(ND_MUL, node, parse_assign(state), tok));
    }
    if (consume(state, "/=")) {
        return to_assign(state, new_binary(ND_DIV, node, parse_assign(state), tok));
    }
    if (consume(state, "%=")) {
        return to_assign(state, new_binary(ND_MOD, node, parse_assign(state), tok));
    }
    if (consume(state, "&=")) {
        return to_assign(state, new_binary(ND_BITAND, node, parse_assign(state), tok));
    }
    if (consume(state, "|=")) {
        return to_assign(state, new_binary(ND_BITOR, node, parse_assign(state), tok));
    }
    if (consume(state, "^=")) {
        return to_assign(state, new_binary(ND_BITXOR, node, parse_assign(state), tok));
    }
    if (consume(state, "<<=")) {
        return to_assign(state, new_binary(ND_SHL, node, parse_assign(state), tok));
    }
    if (consume(state, ">>=")) {
        return to_assign(state, new_binary(ND_SHR, node, parse_assign(state), tok));
    }
    return node;
}

// funcall = ident "(" (assign ("," assign)*)? ")"
static Node* parse_funccall(ParserState* state)
{
    Token* start = read(state);
    VarScope* sc = find_var(state, start);
    if (!sc) {
        error_tok(start, "implicit declaration of function '%s'", start->raw);
    }
    if (!sc->var || sc->var->type->kind != TY_FUNC) {
        error_tok(start, "called object '%s' is not a function", start->raw);
    }
    Type* type = sc->var->type;
    Type* paramType = sc->var->type->params;

    expect(state, "(");
    Array* args = array_new(sizeof(Node), 8);
    while (!consume(state, ")")) {
        if (args->len > 0) {
            expect(state, ",");
        }
        Node* arg = parse_assign(state);
        add_type(arg);

        Token* tok = peek(state);
        if (!paramType && !type->isVariadic) {
            error_tok(tok, "too many arguments");
        }

        if (paramType) {
            if (arg->type->kind == TY_STRUCT) {
                error_tok(arg->tok, "passing struct to function call is not supported");
            }

            arg = new_cast(arg, paramType, tok);
            paramType = paramType->next;
        }

        _array_push_back(args, arg);
    }

    if (paramType) {
        error_tok(peek(state), "too few arguments");
    }

    Node* node = new_node(ND_FUNCCALL, start);
    assert(start->raw);
    node->funcname = strdup(start->raw);
    node->args = args;
    node->type = type->retType;
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

// stmt = "return" expr? ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "for" "(" expr-stmt expr? ";" expr? ")" stmt
//      | "while" "(" expr ")" stmt
//      | "break" ";"
//      | "continue" ";"
//      | "goto" ident ";"
//      | "switch" "(" expr ")" stmt
//      | "case" num ":" stmt
//      | "default" ":" stmt
//      | ident ":" stmt
//      | "{" compound-stmt "}"
//      | expr-stmt
static Node* parse_stmt(ParserState* state)
{
    Token* start = peek(state);

    if (consume(state, "return")) {
        Node* node = new_node(ND_RETURN, start);
        if (consume(state, ";")) {
            return node;
        }
        Node* expr = parse_expr(state);
        expect(state, ";");

        add_type(expr);
        node->lhs = new_cast(expr, state->currentFunc->type->retType, start);
        return node;
    }

    if (consume(state, "if")) {
        Node* node = new_node(ND_IF, start);
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
        enter_scope(state);

        Node* node = new_node(ND_FOR, start);

        char* restoreBrk = state->brkLabel;
        char* restoreCnt = state->cntLabel;
        state->brkLabel = node->brkLabel = new_unique_name();
        state->cntLabel = node->cntLabel = new_unique_name();

        expect(state, "(");

        if (is_type_name(state, peek(state))) {
            Type* baseType = parse_declspec(state, NULL);
            node->init = parse_declaration(state, baseType, NULL);
        } else {
            node->init = parse_expr_stmt(state);
        }

        if (!consume(state, ";")) {
            node->cond = parse_expr(state);
            expect(state, ";");
        }
        if (!consume(state, ")")) {
            node->inc = parse_expr(state);
            expect(state, ")");
        }

        node->then = parse_stmt(state);

        state->brkLabel = restoreBrk;
        state->cntLabel = restoreCnt;

        leave_scope(state);
        return node;
    }

    if (consume(state, "while")) {
        Node* node = new_node(ND_FOR, start);

        char* restoreBrk = state->brkLabel;
        char* restoreCnt = state->cntLabel;
        state->brkLabel = node->brkLabel = new_unique_name();
        state->cntLabel = node->cntLabel = new_unique_name();

        expect(state, "(");
        node->cond = parse_expr(state);
        expect(state, ")");
        node->then = parse_stmt(state);

        state->brkLabel = restoreBrk;
        state->cntLabel = restoreCnt;
        return node;
    }

    if (consume(state, "break")) {
        if (!state->brkLabel) {
            error_tok(start, "break statement not within loop or switch");
        }
        Node* node = new_node(ND_GOTO, start);
        node->uniqueLabel = state->brkLabel;
        expect(state, ";");
        return node;
    }

    if (consume(state, "continue")) {
        if (!state->cntLabel) {
            error_tok(start, "continue statement not within loop");
        }
        Node* node = new_node(ND_GOTO, start);
        node->uniqueLabel = state->cntLabel;
        expect(state, ";");
        return node;
    }

    if (consume(state, "goto")) {
        Token* label = read(state);
        Node* node = new_node(ND_GOTO, label);
        node->label = get_ident(label);
        node->gotoNext = state->gotos;
        state->gotos = node;
        expect(state, ";");
        return node;
    }

    if (consume(state, "switch")) {
        Node* node = new_node(ND_SWITCH, start);
        expect(state, "(");
        node->cond = parse_expr(state);
        expect(state, ")");
        Node* restoreSwitch = state->currentSwitch;
        char* restoreBrk = state->brkLabel;

        state->currentSwitch = node;
        state->brkLabel = node->brkLabel = new_unique_name();
        node->then = parse_stmt(state);

        state->currentSwitch = restoreSwitch;
        state->brkLabel = restoreBrk;
        return node;
    }

    if (consume(state, "case")) {
        if (!state->currentSwitch) {
            error_tok(start, "case label not within a switch statement");
        }

        int64_t val = parse_constexpr(state);
        Node* node = new_node(ND_CASE, start);
        expect(state, ":");
        node->label = new_unique_name();
        node->lhs = parse_stmt(state);
        node->val = val;
        node->caseNext = state->currentSwitch->caseNext;
        state->currentSwitch->caseNext = node;
        return node;
    }

    if (consume(state, "default")) {
        if (!state->currentSwitch) {
            error_tok(start, "'default' label not within a switch statement");
        }

        Node* node = new_node(ND_CASE, start);
        expect(state, ":");
        node->label = new_unique_name();
        node->lhs = parse_stmt(state);
        state->currentSwitch->caseDefault = node;
        return node;
    }

    if (start->kind == TK_IDENT && is_token_equal(peek_n(state, 1), ":")) {
        Node* node = new_node(ND_LABEL, start);
        node->label = strdup(start->raw);
        node->uniqueLabel = new_unique_name();
        read(state);
        expect(state, ":");
        node->lhs = parse_stmt(state);
        node->gotoNext = state->labels;
        state->labels = node;
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
    static Dict* s_lookup;
    if (!s_lookup) {
        static char s_typenames[13][12] = {
            "char", "enum", "extern", "int", "long", "short", "signed", "static", "struct", "typedef", "union", "unsigned", "void"
        };

        s_lookup = dict_new();

        for (size_t i = 0; i < ARRAY_COUNTER(s_typenames); ++i) {
            bool ok = dict_try_add(s_lookup, s_typenames[i], NULL);
            assert(ok);
        }
    }

    if (dict_has_key(s_lookup, tok->raw)) {
        return true;
    }

    return find_typedef(state, tok) != NULL;
}

// compound-stmt = (typedef | declaration | stmt)* "}"
static Node* parse_compound_stmt(ParserState* state)
{
    Token* compoundTok = peek_n(state, -1);
    Node head;
    ZERO_MEMORY(head);
    Node* cur = &head;

    enter_scope(state);

    for (;;) {
        Token* tok = peek(state);

        if (is_token_equal(tok, "}")) {
            read(state);
            break;
        }

        if (is_type_name(state, tok) && !is_token_equal(peek_n(state, 1), ":")) {
            VarAttrib attrib;
            ZERO_MEMORY(attrib);
            Type* baseType = parse_declspec(state, &attrib);

            if (attrib.isTypedef) {
                parse_typedef(state, baseType);
                continue;
            }

            cur = cur->next = parse_declaration(state, baseType, &attrib);
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

// Evaluate a given node as a constant expression.
static int64_t eval(Node* node)
{
    add_type(node);
    switch (node->kind) {
    case ND_ADD:
        return eval(node->lhs) + eval(node->rhs);
    case ND_SUB:
        return eval(node->lhs) - eval(node->rhs);
    case ND_MUL:
        return eval(node->lhs) * eval(node->rhs);
    case ND_DIV:
        if (node->type->isUnsigned) {
            return (uint64_t)eval(node->lhs) / eval(node->rhs);
        }
        return eval(node->lhs) / eval(node->rhs);
    case ND_NEG:
        return -eval(node->lhs);
    case ND_MOD:
        return eval(node->lhs) % eval(node->rhs);
    case ND_BITAND:
        return eval(node->lhs) & eval(node->rhs);
    case ND_BITOR:
        return eval(node->lhs) | eval(node->rhs);
    case ND_BITXOR:
        return eval(node->lhs) ^ eval(node->rhs);
    case ND_SHL:
        return eval(node->lhs) << eval(node->rhs);
    case ND_SHR:
        return eval(node->lhs) >> eval(node->rhs);
    case ND_EQ:
        return eval(node->lhs) == eval(node->rhs);
    case ND_NE:
        return eval(node->lhs) != eval(node->rhs);
    case ND_LT:
        return eval(node->lhs) < eval(node->rhs);
    case ND_LE:
        return eval(node->lhs) <= eval(node->rhs);
    case ND_TERNARY:
        return eval(node->cond) ? eval(node->then) : eval(node->els);
    case ND_COMMA:
        return eval(node->rhs);
    case ND_NOT:
        return !eval(node->lhs);
    case ND_BITNOT:
        return ~eval(node->lhs);
    case ND_LOGAND:
        return eval(node->lhs) && eval(node->rhs);
    case ND_LOGOR:
        return eval(node->lhs) || eval(node->rhs);
    case ND_CAST:
        if (is_integer(node->type)) {
            switch (node->type->size) {
            case 1:
                return (uint8_t)eval(node->lhs);
            case 2:
                return (uint16_t)eval(node->lhs);
            case 4:
                return (uint32_t)eval(node->lhs);
            }
        }
        return eval(node->lhs);
    case ND_NUM:
        return node->val;
    default:
        break;
    }

    error_tok(node->tok, "not a compile-time constant");
    return 0;
}

static int64_t parse_constexpr(ParserState* state)
{
    Node* node = parse_ternary(state);
    return eval(node);
}

// declspec = ("void" | "_Bool" | "char" | "short" | "int" | "long"
//             | "typedef" | "static" | "extern"
//             | "signed" | "unsigned"
//             | struct-decl | union-decl | typedef-name
//             | enum-specifier)+
static Type* parse_declspec(ParserState* state, VarAttrib* attrib)
{
    enum {
        VOID = 1 << 0,
        BOOL = 1 << 2,
        CHAR = 1 << 4,
        SHORT = 1 << 6,
        INT = 1 << 8,
        LONG = 1 << 10,
        OTHER = 1 << 12,
        SIGNED = 1 << 13,
        UNSIGNED = 1 << 14,
    };

    Token* tok = NULL;
    Type* ty = &g_int_type;
    int counter = 0;

    for (;;) {
        tok = peek(state);
        if (!is_type_name(state, tok)) {
            break;
        }

        // Handle storage class specifiers.
        bool isTypedef = is_token_equal(tok, "typedef");
        bool isStatic = is_token_equal(tok, "static");
        bool isExtern = is_token_equal(tok, "extern");
        if (isTypedef || isStatic || isExtern) {
            if (!attrib) {
                error_tok(tok, "storage class specifier is not allowed in this context");
            }

            attrib->isTypedef = !!(attrib->isTypedef || isTypedef);
            attrib->isStatic = !!(attrib->isStatic || isStatic);
            attrib->isExtern = !!(attrib->isExtern || isExtern);
            if (attrib->isStatic + attrib->isTypedef + attrib->isExtern > 1) {
                error_tok(tok, "multiple storage classes in declaration specifiers");
            }
            read(state);
            continue;
        }

        // user defined types
        Type* ty2 = find_typedef(state, tok);
        bool isStruct = is_token_equal(tok, "struct");
        bool isUnion = is_token_equal(tok, "union");
        bool isEnum = is_token_equal(tok, "enum");
        if (isStruct || isUnion || isEnum || ty2) {
            if (counter) {
                break;
            }

            read(state);
            if (isStruct) {
                ty = parse_struct_decl(state);
            } else if (isUnion) {
                ty = parse_union_decl(state);
            } else if (isEnum) {
                ty = parse_enum_specifier(state);
            } else {
                ty = ty2;
            }

            counter += OTHER;
            continue;
        }

        if (consume(state, "void")) {
            counter += VOID;
        } else if (consume(state, "char")) {
            counter += CHAR;
        } else if (consume(state, "short")) {
            counter += SHORT;
        } else if (consume(state, "int")) {
            counter += INT;
        } else if (consume(state, "long")) {
            counter += LONG;
        } else if (consume(state, "signed")) {
            counter |= SIGNED;
        } else if (consume(state, "unsigned")) {
            counter |= UNSIGNED;
        } else {
            assert(0);
        }

        switch (counter) {
        case VOID:
            ty = &g_void_type;
            break;
        case CHAR:
        case SIGNED + CHAR:
            ty = &g_char_type;
            break;
        case UNSIGNED + CHAR:
            ty = &g_uchar_type;
            break;
        case SHORT:
        case SIGNED + SHORT:
        case SHORT + INT:
        case SIGNED + SHORT + INT:
            ty = &g_short_type;
            break;
        case UNSIGNED + SHORT:
        case UNSIGNED + SHORT + INT:
            ty = &g_ushort_type;
            break;
        case INT:
        case SIGNED:
        case SIGNED + INT:
            ty = &g_int_type;
            break;
        case UNSIGNED:
        case UNSIGNED + INT:
            ty = &g_uint_type;
            break;
        case LONG:
        case LONG + INT:
        case LONG + LONG:
        case LONG + LONG + INT:
        case SIGNED + LONG:
        case SIGNED + LONG + INT:
        case SIGNED + LONG + LONG:
        case SIGNED + LONG + LONG + INT:
            ty = &g_long_type;
            break;
        case UNSIGNED + LONG:
        case UNSIGNED + LONG + INT:
        case UNSIGNED + LONG + LONG:
        case UNSIGNED + LONG + LONG + INT:
            ty = &g_ulong_type;
            break;
        default:
            error_tok(tok, "invalid type specifer");
        }
    }

    return ty;
}

// enum-specifier = ident? "{" enum-list? "}"
//                | ident ("{" enum-list? "}")?
//
// enum-list      = ident ("=" num)? ("," ident ("=" num)?)*
static Type* parse_enum_specifier(ParserState* state)
{
    Type* ty = enum_type();

    // Read a struct tag.
    Token* tag = NULL;
    Token* start = peek(state);
    if (start->kind == TK_IDENT) {
        tag = start;
        read(state);
    }

    if (tag && !equal(state, "{")) {
        Type* ty = find_tag(state, tag);
        if (!ty) {
            error_tok(tag, "'%s' is not enum", tag->raw);
        }
        if (ty->kind != TY_ENUM) {
            error_tok(tag, "'%s' defined as wrong kind of tag", tag->raw);
        }
        return ty;
    }

    expect(state, "{");

    // Read an enum-list.
    int i = 0;
    int val = 0;
    while (!consume(state, "}")) {
        if (i++ > 0) {
            expect(state, ",");
            if (consume(state, "}")) {
                break;
            }
        }

        char* name = get_ident(read(state));
        if (consume(state, "=")) {
            val = parse_constexpr(state);
        }

        VarScope* sc = push_var_scope(state, name);
        sc->enumType = ty;
        sc->enumVal = val++;
    }

    if (tag) {
        push_tag_scope(state, tag, ty);
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

// func-params = ("void" | param ("," param)* ("," "...")?)? ")"
// param       = declspec declarator
static Type* parse_func_params(ParserState* state, Type* type)
{
    Type head;
    ZERO_MEMORY(head);
    Type* cur = &head;
    bool isVaridadic = false;
    while (!consume(state, ")")) {
        if (cur != &head) {
            expect(state, ",");
        }

        if (consume(state, "...")) {
            isVaridadic = true;
            expect(state, ")");
            break;
        }

        Type* basety = parse_declspec(state, NULL);
        Type* ty = parse_declarator(state, basety);
        cur = cur->next = copy_type(ty);
    }
    type = func_type(type);
    type->params = head.next;
    type->isVariadic = isVaridadic;
    return type;
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
        int arrayLen = parse_constexpr(state);
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
static Node* parse_declaration(ParserState* state, Type* baseType, VarAttrib* attrib)
{
    Node head;
    ZERO_MEMORY(head);
    Node* cur = &head;
    int i = 0;
    while (!consume(state, ";")) {
        if (i++ > 0) {
            expect(state, ",");
        }

        Type* type = parse_declarator(state, baseType);
        if (type->kind == TY_VOID) {
            error_tok(type->name, "variable or field '%s' declared void", type->name->raw);
        }

        if (attrib && attrib->isStatic) {
            // static local variable
            Obj* var = new_anon_gvar(state, type);
            push_var_scope(state, get_ident(type->name))->var = var;
            if (consume(state, "=")) {
                parse_gvar_initializer(state, var);
            }
            continue;
        }

        Obj* var = new_lvar(state, get_ident(type->name), type);

        Token* tok = peek(state);
        if (is_token_equal(tok, "=")) {
            read(state);

            Node* expr = parse_lvar_initializer(state, var);
            cur = cur->next = new_unary(ND_EXPR_STMT, expr, tok);
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

static void write_buf(char* buf, uint64_t val, int sz)
{
    if (sz == 1) {
        *buf = val;
    } else if (sz == 2) {
        *(uint16_t*)buf = val;
    } else if (sz == 4) {

        *(uint32_t*)buf = val;
    } else if (sz == 8) {

        *(uint64_t*)buf = val;
    } else {
        assert(0);
    }
}

static void write_gvar_data(Initializer* init, Type* ty, char* buf, int offset)
{
    if (ty->kind == TY_ARRAY) {
        int sz = ty->base->size;
        for (int i = 0; i < ty->arrayLen; i++) {
            write_gvar_data(init->children[i], ty->base, buf, offset + sz * i);
        }
        return;
    }
    if (init->expr) {
        write_buf(buf + offset, eval(init->expr), ty->size);
    }
    if (ty->kind == TY_STRUCT) {
        for (Member* mem = ty->members; mem; mem = mem->next) {
            write_gvar_data(init->children[mem->idx], mem->type, buf, offset + mem->offset);
        }
        return;
    }
}

// Initializers for global variables are evaluated at compile-time and
// embedded to .data section. This function serializes Initializer
// objects to a flat byte array. It is a compile error if an
// initializer list contains a non-constant expression.
static void parse_gvar_initializer(ParserState* state, Obj* var)
{
    Initializer* init = parse_initializer(state, var->type);
    char* buf = calloc(1, var->type->size);
    write_gvar_data(init, var->type, buf, 0);
    var->initData = buf;
}

static void parse_global_variable(ParserState* state, Type* basety, VarAttrib* attrib)
{
    int i = 0;
    while (!consume(state, ";")) {
        if (i++ > 0) {
            expect(state, ",");
        }

        Type* type = parse_declarator(state, basety);
        Obj* obj = new_gvar(state, get_ident(type->name), type);
        obj->isStatic = attrib->isStatic;
        obj->isDefinition = !attrib->isExtern;

        if (consume(state, "=")) {
            parse_gvar_initializer(state, obj);
        }
    }
}

static void skip_excess_element(ParserState* state)
{
    warn_tok(peek(state), "excess elements in initializer");
    if (consume(state, "{")) {
        skip_excess_element(state);
        expect(state, "}");
        return;
    }

    parse_assign(state);
}

static Initializer* new_initializer(Type* ty, bool flex)
{
    (void)flex;

    Initializer* init = calloc(1, ALIGN(sizeof(Initializer), 16));
    init->type = ty;
    if (ty->kind == TY_ARRAY) {
        int size = ty->arrayLen * sizeof(Initializer);
        size = ALIGN(size, 16);
        init->children = calloc(1, size);
        for (int i = 0; i < ty->arrayLen; ++i) {
            init->children[i] = new_initializer(ty->base, false);
        }
    }

    if (ty->kind == TY_STRUCT) {
        // Count the number of struct members.
        int len = 0;
        for (Member* mem = ty->members; mem; mem = mem->next) {
            len++;
        }
        init->children = calloc(len, sizeof(Initializer*));
        for (Member* mem = ty->members; mem; mem = mem->next) {
            init->children[mem->idx] = new_initializer(mem->type, false);
        }
        return init;
    }
    return init;
}

// string-initializer = string-literal
static void string_initializer(ParserState* state, Initializer* init)
{
    Token* tok = read(state);
    assert(tok->kind == TK_STR);
    int len = MIN(init->type->arrayLen, tok->type->arrayLen);
    for (int i = 0; i < len; i++) {
        init->children[i]->expr = new_num(tok->str[i], tok);
    }
}

// array-initializer = "{" initializer ("," initializer)* "}"
static void array_initializer(ParserState* state, Initializer* init)
{
    expect(state, "{");
    for (int i = 0; !consume(state, "}"); i++) {
        if (i > 0) {
            expect(state, ",");
        }
        if (i < init->type->arrayLen) {
            initializer2(state, init->children[i]);
        } else {
            skip_excess_element(state);
        }
    }
}

// struct-initializer = "{" initializer ("," initializer)* "}"
static void struct_initializer(ParserState* state, Initializer* init)
{
    expect(state, "{");
    Member* mem = init->type->members;
    while (!consume(state, "}")) {
        if (mem != init->type->members) {
            expect(state, ",");
        }
        if (mem) {
            initializer2(state, init->children[mem->idx]);
            mem = mem->next;
        } else {
            skip_excess_element(state);
        }
    }
}

// initializer = string-initializer | array-initializer | assign
static void initializer2(ParserState* state, Initializer* init)
{
    Token* start = peek(state);
    int kind = init->type->kind;
    if (kind == TY_ARRAY && start->kind == TK_STR) {
        string_initializer(state, init);
        return;
    }
    if (kind == TY_ARRAY) {
        array_initializer(state, init);
        return;
    }
    if (kind == TY_STRUCT) {
        // A struct can be initialized with another struct. E.g.
        // `struct T x = y;` where y is a variable of type `struct T`.
        // Handle that case first.
        if (!equal(state, "{")) {
            Node* expr = parse_assign(state);
            add_type(expr);
            if (expr->type->kind == TY_STRUCT) {
                init->expr = expr;
                return;
            }
        }

        struct_initializer(state, init);
        return;
    }

    init->expr = parse_assign(state);
}

static Initializer* parse_initializer(ParserState* state, Type* ty)
{
    Initializer* init = new_initializer(ty, false);
    initializer2(state, init);
    return init;
}

static Node* init_desg_expr(InitDesg* desg, Token* tok)
{
    if (desg->var) {
        return new_var(desg->var, tok);
    }

    if (desg->member) {
        Node* node = new_unary(ND_MEMBER, init_desg_expr(desg->next, tok), tok);
        node->member = desg->member;
        return node;
    }

    Node* lhs = init_desg_expr(desg->next, tok);
    Node* rhs = new_num(desg->idx, tok);
    return new_unary(ND_DEREF, new_add(lhs, rhs, tok), tok);
}

static Node* create_lvar_init(Initializer* init, Type* ty, InitDesg* desg, Token* tok)
{
    if (ty->kind == TY_ARRAY) {
        Node* node = new_node(ND_NULL_EXPR, tok);
        for (int i = 0; i < ty->arrayLen; i++) {
            InitDesg desg2 = { desg, i };
            Node* rhs = create_lvar_init(init->children[i], ty->base, &desg2, tok);
            node = new_binary(ND_COMMA, node, rhs, tok);
        }
        return node;
    }

    if (ty->kind == TY_STRUCT && !init->expr) {
        Node* node = new_node(ND_NULL_EXPR, tok);
        for (Member* mem = ty->members; mem; mem = mem->next) {
            InitDesg desg2 = { desg, 0, mem, NULL };
            Node* rhs = create_lvar_init(init->children[mem->idx], mem->type, &desg2, tok);
            node = new_binary(ND_COMMA, node, rhs, tok);
        }
        return node;
    }

    if (!init->expr) {
        return new_node(ND_NULL_EXPR, tok);
    }

    Node* lhs = init_desg_expr(desg, tok);
    return new_binary(ND_ASSIGN, lhs, init->expr, tok);
}

// A variable definition with an initializer is a shorthand notation
// for a variable definition followed by assignments. This function
// generates assignment expressions for an initializer. For example,
// `int x[2][2] = {{6, 7}, {8, 9}}` is converted to the following
// expressions:
//
//   x[0][0] = 6;
//   x[0][1] = 7;
//   x[1][0] = 8;
//   x[1][1] = 9;
static Node* parse_lvar_initializer(ParserState* state, Obj* var)
{
    Token* tok = peek(state);
    Initializer* init = parse_initializer(state, var->type);
    InitDesg desg;
    ZERO_MEMORY(desg);
    desg.var = var;

    // If a partial initializer list is given, the standard requires
    // that unspecified elements are set to 0. Here, we simply
    // zero-initialize the entire memory region of a variable before
    // initializing it with user-supplied values.
    Node* lhs = new_node(ND_MEMZERO, tok);
    lhs->var = var;
    Node* rhs = create_lvar_init(init, var->type, &desg, tok);
    return new_binary(ND_COMMA, lhs, rhs, tok);
}

// This function matches gotos with labels.
//
// We cannot resolve gotos as we parse a function because gotos
// can refer a label that appears later in the function.
// So, we need to do this after we parse the entire function.
static void resolve_goto_labels(ParserState* state)
{
    for (Node* x = state->gotos; x; x = x->gotoNext) {
        for (Node* y = state->labels; y; y = y->gotoNext) {
            if (streq(x->label, y->label)) {
                x->uniqueLabel = y->uniqueLabel;
                break;
            }
        }
        if (x->uniqueLabel == NULL) {
            error_tok(x->tok, "label '%s' used but not defined", x->label);
        }
    }

    state->gotos = state->labels = NULL;
}

static Obj* parse_function(ParserState* state, Type* basetpye, VarAttrib* attrib)
{
    Type* type = parse_declarator(state, basetpye);
    Obj* fn = new_gvar(state, get_ident(type->name), type);
    fn->isFunc = true;
    fn->isDefinition = !consume(state, ";");
    fn->isStatic = attrib->isStatic;
    if (!fn->isDefinition) {
        return fn;
    }

    state->locals = NULL;
    state->currentFunc = fn;

    enter_scope(state);

    create_param_lvars(state, type->params);
    fn->params = state->locals;
    if (type->isVariadic) {
        fn->vaArea = new_lvar(state, "__va_area__", array_of(&g_char_type, 136));
    }
    expect(state, "{");
    fn->body = parse_compound_stmt(state);
    fn->locals = state->locals;

    leave_scope(state);
    resolve_goto_labels(state);
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
    return type->kind == TY_FUNC;
}

// program = (typedef | function-definition | global-variable)*
Obj* parse(List* tokens)
{
    ParserState state;
    memset(&state, 0, sizeof(ParserState));
    state.reader.tokens = tokens;
    state.reader.cursor = tokens->front;

    enter_scope(&state);

    state.globals = NULL;
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
            parse_function(&state, baseType, &attrib);
        } else {
            parse_global_variable(&state, baseType, &attrib);
        }
    }

    leave_scope(&state);
    assert(state.scopes.len == 0);
    return state.globals;
}
