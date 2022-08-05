#include "minic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AS_TOK(LIST_NODE) (Token*)((LIST_NODE) + 1)

// All local variable instances created during parsing are
// accumulated to this list.
Obj* locals;

// Find a local variable by name.
static Obj* find_var(Token const* tok)
{
    for (Obj* var = locals; var; var = var->next) {
        if ((int)strlen(var->name) == tok->len && !strncmp(tok->start, var->name, tok->len)) {
            return var;
        }
    }
    return nullptr;
}

int token_as_int(Token const* tok)
{
    assert(tok->eTokenKind == TK_NUM);
    return atoi(tok->start);
}

static Node* new_node(NodeKind eNodeKind, Token const* tok)
{
    Node* node = calloc(1, sizeof(Node));
    node->tok = tok;
    node->eNodeKind = eNodeKind;
    return node;
}

static Node* new_num_node(int val, Token const* tok)
{
    Node* node = new_node(ND_NUM, tok);
    node->eNodeKind = ND_NUM;
    node->val = val;
    return node;
}

static Node* new_var_node(Obj* var, Token const* tok)
{
    Node* node = new_node(ND_VAR, tok);
    node->var = var;
    return node;
}

static Obj* new_lvar(char* name)
{
    Obj* var = calloc(1, sizeof(Obj));
    var->name = name;
    var->next = locals;
    locals = var;
    return var;
}

static Node* new_binary_node(NodeKind eNodeKind, Node* lhs, Node* rhs, Token const* tok)
{
    Node* node = new_node(eNodeKind, tok);
    node->isBinary = true;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node* new_unary_node(NodeKind eNodeKind, Node* expr, Token const* tok)
{
    Node* node = new_node(eNodeKind, tok);
    node->isUnary = true;
    node->lhs = expr;
    return node;
}

static void tok_consume(ListNode** pToks)
{
    assert(pToks && *pToks);
    *pToks = (*pToks)->next;
}

// if equal, consume token
static bool tok_eq(ListNode** pToks, const char* expect)
{
    assert(pToks && *pToks);

    Token const* token = AS_TOK(*pToks);
    int const expectLen = strlen(expect);
    if (expectLen != token->len) {
        return false;
    }

    bool const isEqual = strncmp(token->start, expect, expectLen) == 0;
    if (isEqual) {
        *pToks = (*pToks)->next;
        return true;
    }

    return false;
}

static void tok_expect(ListNode** pToks, char const* expect)
{
    assert(pToks && *pToks);
    Token const* token = AS_TOK(*pToks);
    int const expectLen = strlen(expect);
    if (expectLen != token->len || (strncmp(token->start, expect, token->len) != 0)) {
        error_at_token(token, "expected '%s'", expect);
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

// primary = "(" expr ")" | num | ident
static Node* parse_primary(ListNode** pToks)
{
    if (tok_eq(pToks, "(")) {
        Node* node = parse_add(pToks);
        tok_expect(pToks, ")"); // consume ')'
        return node;
    }

    Token const* tok = AS_TOK(*pToks);
    if (tok->eTokenKind == TK_NUM) {
        Node* node = new_num_node(token_as_int(tok), tok);
        tok_consume(pToks);
        return node;
    }

    if (tok->eTokenKind == TK_IDENT) {
        Obj* var = find_var(tok);
        if (!var) {
            // if not found, create
            var = new_lvar(strnduplicate(tok->start, tok->len));
        }

        tok_consume(pToks);
        return new_var_node(var, tok);
    }

    error_at_token(tok, "expected expression before '%.*s' token", tok->len, tok->start);
    return nullptr;
}

// unary = ("+" | "-" | "*" | "&") unary
//       | primary
static Node* parse_unary(ListNode** pToks)
{
    if (tok_eq(pToks, "+")) {
        return parse_unary(pToks);
    }

    Token* tok = AS_TOK(*pToks);

    if (tok_eq(pToks, "-")) {
        return new_unary_node(ND_NEG, parse_unary(pToks), tok);
    }

    if (tok_eq(pToks, "*")) {
        return new_unary_node(ND_DEREF, parse_unary(pToks), tok);
    }

    if (tok_eq(pToks, "&")) {
        return new_unary_node(ND_ADDR, parse_unary(pToks), tok);
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
            if (tok_eq(pToks, *p)) {
                NodeKind kind = to_binary_node_kind(*p);
                assert(kind != ND_INVALID);
                node = new_binary_node(kind, node, fp(pToks), AS_TOK(pToks[0]->prev));
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

// expr = mul ("+" mul | "-" mul)*
static Node* parse_add(ListNode** pToks)
{
    static char const* s_symbols[] = { "+", "-", nullptr };
    return parse_binary_internal(pToks, s_symbols, parse_mul);
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
    if (tok_eq(pToks, "=")) {
        node = new_binary_node(ND_ASSIGN, node, parse_assign(pToks), AS_TOK(pToks[0]->prev));
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
    Token const* tok = AS_TOK(*pToks);

    if (tok_eq(pToks, ";")) {
        return new_node(ND_BLOCK, tok);
    }
    Node* node = new_unary_node(ND_EXPR_STMT, parse_expr(pToks), tok);
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
    Token const* tok = AS_TOK(*pToks);

    if (tok_eq(pToks, "return")) {
        Node* node = new_unary_node(ND_RETURN, parse_expr(pToks), tok);
        tok_expect(pToks, ";");
        return node;
    }

    if (tok_eq(pToks, "if")) {
        Node* node = new_node(ND_IF, tok);
        tok_expect(pToks, "(");
        node->cond = parse_expr(pToks);
        tok_expect(pToks, ")");
        node->then = parse_stmt(pToks);
        if (tok_eq(pToks, "else")) {
            node->els = parse_stmt(pToks);
        }
        return node;
    }

    if (tok_eq(pToks, "for")) {
        Node* node = new_node(ND_FOR, tok);

        tok_expect(pToks, "(");
        node->init = parse_expr_stmt(pToks);
        if (!tok_eq(pToks, ";")) {
            node->cond = parse_expr(pToks);
            tok_expect(pToks, ";");
        }
        if (!tok_eq(pToks, ")")) {
            node->inc = parse_expr(pToks);
            tok_expect(pToks, ")");
        }

        node->then = parse_stmt(pToks);
        return node;
    }

    if (tok_eq(pToks, "while")) {
        Node* node = new_node(ND_FOR, tok);
        tok_expect(pToks, "(");
        node->cond = parse_expr(pToks);
        tok_expect(pToks, ")");
        node->then = parse_stmt(pToks);
        return node;
    }

    if (tok_eq(pToks, "{")) {
        return parse_compound_stmt(pToks);
    }

    return parse_expr_stmt(pToks);
}

// compound-stmt = stmt* "}"
static Node* parse_compound_stmt(ListNode** pToks)
{
    Token* tok = AS_TOK(pToks[0]->prev);
    Node head;
    Node* cur = &head;
    while (!tok_eq(pToks, "}")) {
        cur = cur->next = parse_stmt(pToks);
    }

    Node* node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    return node;
}

Function* parse(List* toks)
{
    ListNode* iter = toks->front;

    Function* prog = calloc(1, sizeof(Function));
    tok_expect(&iter, "{");
    prog->body = parse_compound_stmt(&iter);
    prog->locals = locals;
    return prog;
}
