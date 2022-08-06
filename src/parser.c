#include "minic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Create Node API
 */
static Node* new_node(NodeKind eNodeKind, Token const* tok)
{
    Node* node = calloc(1, sizeof(Node));
    node->tok = tok;
    node->eNodeKind = eNodeKind;
    return node;
}

static Node* new_num(int val, Token const* tok)
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

// All local variable instances created during parsing are
// accumulated to this list.
Obj* g_locals;

static Obj* new_lvar(const char* name, Type* type)
{
    Obj* var = calloc(1, sizeof(Obj));
    var->name = name;
    var->type = type;
    var->next = g_locals;
    g_locals = var;
    return var;
}

typedef Node* (*ParseBinaryFn)(ListNode**);

// Find a local variable by name.
static Obj* find_var(Token const* tok)
{
    for (Obj* var = g_locals; var; var = var->next) {
        if ((int)strlen(var->name) == tok->len && !strncmp(tok->start, var->name, tok->len)) {
            return var;
        }
    }
    return nullptr;
}

static Token* as_tok(ListNode* listnode)
{
    return (Token*)(listnode + 1);
}

int token_as_int(Token const* tok)
{
    assert(tok->eTokenKind == TK_NUM);
    return atoi(tok->start);
}

static void tok_shift(ListNode** pToks)
{
    assert(pToks && *pToks && (*pToks)->next);
    *pToks = (*pToks)->next;
}

static void tok_unshift(ListNode** pToks)
{
    assert(pToks && *pToks && (*pToks)->prev);
    *pToks = (*pToks)->prev;
}

static bool tok_eq(ListNode** pToks, const char* expect)
{
    assert(pToks && *pToks);

    const Token* tok = as_tok(*pToks);
    const int expectLen = (int)strlen(expect);
    if (expectLen != tok->len) {
        return false;
    }

    return strncmp(tok->start, expect, expectLen) == 0;
}

static bool tok_consume(ListNode** pToks, const char* expect)
{
    const bool equal = tok_eq(pToks, expect);
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

static Node* parse_primary(ListNode** pToks);
static Node* parse_unary(ListNode** pToks);
static Node* parse_mul(ListNode** pToks);
static Node* parse_add(ListNode** pToks);
static Node* parse_relational(ListNode** pToks);
static Node* parse_equality(ListNode** pToks);
static Node* parse_assign(ListNode** pToks);
static Node* parse_expr(ListNode** pToks);
static Node* parse_expr_stmt(ListNode** pToks);
static Node* parse_compound_stmt(ListNode** pToks);
static Node* parse_decl(ListNode** pToks);

#define TOKSTR(TOK) (TOK)->len, (TOK)->start

// primary = "(" expr ")" | num | ident
static Node* parse_primary(ListNode** pToks)
{
    if (tok_consume(pToks, "(")) {
        Node* node = parse_add(pToks);
        tok_expect(pToks, ")"); // consume ')'
        return node;
    }

    Token const* tok = as_tok(*pToks);
    if (tok->eTokenKind == TK_NUM) {
        Node* node = new_num(token_as_int(tok), tok);
        tok_shift(pToks);
        return node;
    }

    if (tok->eTokenKind == TK_IDENT) {
        Obj* var = find_var(tok);
        if (!var) {
            // if not found, create
            // var = new_lvar(strnduplicate(tok->start, tok->len));
            error_tok(tok, "undefined variable '%.*s'", TOKSTR(tok));
        }

        tok_shift(pToks);
        return new_var(var, tok);
    }

    error_tok(tok, "expected expression before '%.*s' token", TOKSTR(tok));
    return nullptr;
}

// unary = ("+" | "-" | "*" | "&") unary
//       | primary
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

    return parse_primary(pToks);
}

static bool streq(char const* a, char const* b)
{
    return strcmp(a, b) == 0;
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

static Node* new_add_node(Node* lhs, Node* rhs, Token* tok)
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
    rhs = new_binary(ND_MUL, rhs, new_num(8, tok), tok);
    return new_binary(ND_ADD, lhs, rhs, tok);
}

static Node* new_sub_node(Node* lhs, Node* rhs, Token* tok)
{
    add_type(lhs);
    add_type(rhs);

    // num - num
    if (is_integer(lhs->type) && is_integer(rhs->type)) {

        return new_binary(ND_SUB, lhs, rhs, tok);
    }

    // ptr - num
    if (lhs->type->base && is_integer(rhs->type)) {
        rhs = new_binary(ND_MUL, rhs, new_num(8, tok), tok);
        add_type(rhs);
        Node* node = new_binary(ND_SUB, lhs, rhs, tok);
        node->type = lhs->type;
        return node;
    }
    // ptr - ptr, which returns how many elements are between the two.
    if (lhs->type->base && rhs->type->base) {
        Node* node = new_binary(ND_SUB, lhs, rhs, tok);
        node->type = g_int_type;
        return new_binary(ND_DIV, node, new_num(8, tok), tok);
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
            node = new_add_node(node, parse_mul(pToks), start);
            continue;
        }

        if (tok_consume(pToks, "-")) {
            node = new_sub_node(node, parse_mul(pToks), start);
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

// expr = assign
static Node* parse_expr(ListNode** pToks)
{
    return parse_assign(pToks);
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

// compound-stmt = (declaration | stmt)* "}"
static Node* parse_compound_stmt(ListNode** pToks)
{
    Token* tok = as_tok(pToks[0]->prev);
    Node head;
    memset(&head, 0, sizeof(Node));
    Node* cur = &head;
    while (!tok_consume(pToks, "}")) {
        if (tok_eq(pToks, "int")) {
            cur = cur->next = parse_decl(pToks);
        } else {
            cur = cur->next = parse_stmt(pToks);
        }
        add_type(cur);
    }

    Node* node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    return node;
}

// declspec = "int"
static Type* parse_declspec(ListNode** pToks)
{
    if (tok_consume(pToks, "int")) {
        return g_int_type;
    }

    unreachable();
    return nullptr;
}

// declarator = "*"* ident
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
    type->name = tok;
    return type;
}

static const char* get_ident(const Token* tok)
{
    if (tok->eTokenKind != TK_IDENT) {
        error_tok(tok, "expected an identifier");
    }
    return strnduplicate(tok->start, tok->len);
}

// declaration = declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
static Node* parse_decl(ListNode** pToks)
{
    Type* base_type = parse_declspec(pToks);
    Node head;
    memset(&head, 0, sizeof(Node));
    Node* cur = &head;
    int i = 0;
    while (!tok_consume(pToks, ";")) {
        if (i++ > 0) {
            tok_expect(pToks, ",");
        }
        Type* type = parse_declarator(pToks, base_type);
        Obj* var = new_lvar(get_ident(type->name), type);
        if (tok_eq(pToks, "=")) {
            tok_unshift(pToks); // unshift the var name back
            Node* lhs = new_var(var, type->name);
            Node* rhs = parse_assign(pToks);
            Node* node = new_binary(ND_ASSIGN, lhs, rhs, as_tok(pToks[0]->prev));
            cur = cur->next = new_unary(ND_EXPR_STMT, node, as_tok(*pToks));
        }
    }

    Node* node = new_node(ND_BLOCK, as_tok(*pToks));
    node->body = head.next;
    return node;
}

Function* parse(List* toks)
{
    ListNode* iter = toks->front;

    Function* prog = calloc(1, sizeof(Function));
    tok_expect(&iter, "{");
    prog->body = parse_compound_stmt(&iter);
    prog->locals = g_locals;
    return prog;
}
