#include "minic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int token_as_int(Token const* token)
{
    assert(token->eTokenKind == TK_NUM);
    return atoi(token->start);
}

static Node* make_node(NodeKind eNodeKind)
{
    Node* node = calloc(1, sizeof(Node));
    node->eNodeKind = eNodeKind;
    return node;
}

static Node* make_num(int val)
{
    Node* node = make_node(ND_NUM);
    node->eNodeKind = ND_NUM;
    node->val = val;
    return node;
}

static Node* make_binary(NodeKind eNodeKind, Node* lhs, Node* rhs)
{
    Node* node = make_node(eNodeKind);
    node->isBinary = true;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node* make_unary(NodeKind eNodeKind, Node* rhs)
{
    Node* node = make_node(eNodeKind);
    node->isUnary = true;
    node->rhs = rhs;
    return node;
}

static void tok_consume(ListNode** pToks)
{
    assert(pToks && *pToks);
    *pToks = (*pToks)->next;
}

static bool tok_equal_then_consume(ListNode** pToks, const char* expect)
{
    assert(pToks && *pToks);

    Token const* token = (Token const*)(*pToks + 1);
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
    Token const* token = (Token const*)(*pToks + 1);
    int const expectLen = strlen(expect);
    if (expectLen != token->len || (strncmp(token->start, expect, token->len) != 0)) {
        error_at_token(token, "expected '%s'", expect);
    }

    *pToks = (*pToks)->next;
}

static Node* parse_expr(ListNode** pToks);
static Node* parse_equality(ListNode** pToks);
static Node* parse_relational(ListNode** pToks);
static Node* parse_add(ListNode** pToks);
static Node* parse_unary(ListNode** pToks);
static Node* parse_primary(ListNode** pToks);

// primary = "(" expr ")" | num
static Node* parse_primary(ListNode** pToks)
{
    if (tok_equal_then_consume(pToks, "(")) {
        Node* node = parse_add(pToks);
        tok_expect(pToks, ")"); // consume ')'
        return node;
    }

    Token const* tok = (Token const*)(*pToks + 1);
    if (tok->eTokenKind == TK_NUM) {
        Node* node = make_num(token_as_int(tok));
        tok_consume(pToks);
        return node;
    }

    error_at_token(tok, "expected expression, got '%.*s'", tok->len, tok->start);
    return nullptr;
}

// unary = ("+" | "-") unary
//       | primary
static Node* parse_unary(ListNode** pToks)
{
    if (tok_equal_then_consume(pToks, "+")) {
        return parse_unary(pToks);
    }

    if (tok_equal_then_consume(pToks, "-")) {
        return make_unary(ND_NEG, parse_unary(pToks));
    }

    return parse_primary(pToks);
}

// mul = unary ("*" unary | "/" unary)*
static Node* parse_mul(ListNode** pToks)
{
    Node* node = parse_unary(pToks);

    for (;;) {
        if (tok_equal_then_consume(pToks, "*")) {
            node = make_binary(ND_MUL, node, parse_unary(pToks));
            continue;
        }

        if (tok_equal_then_consume(pToks, "/")) {
            node = make_binary(ND_DIV, node, parse_unary(pToks));
            continue;
        }

        if (tok_equal_then_consume(pToks, "%")) {
            node = make_binary(ND_REM, node, parse_unary(pToks));
            continue;
        }

        return node;
    }
}

// expr = mul ("+" mul | "-" mul)*
static Node* parse_add(ListNode** pToks)
{
    Node* node = parse_mul(pToks);

    for (;;) {
        if (tok_equal_then_consume(pToks, "+")) {
            node = make_binary(ND_ADD, node, parse_mul(pToks));
            continue;
        }

        if (tok_equal_then_consume(pToks, "-")) {
            node = make_binary(ND_SUB, node, parse_mul(pToks));
            continue;
        }

        return node;
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *parse_relational(ListNode** pToks) {
    Node *node = parse_add(pToks);

    for (;;) {
        if (tok_equal_then_consume(pToks, "<")) {
            node = make_binary(ND_LT, node, parse_add(pToks));
            continue;
        }

        if (tok_equal_then_consume(pToks, "<=")) {
            node = make_binary(ND_LE, node, parse_add(pToks));
            continue;
        }

        if (tok_equal_then_consume(pToks, ">")) {
            node = make_binary(ND_GT, node, parse_add(pToks));
            continue;
        }

        if (tok_equal_then_consume(pToks, ">=")) {
            node = make_binary(ND_GE, node, parse_add(pToks));
            continue;
        }

        return node;
    }
}

// equality = relational ("==" relational | "!=" relational)*
static Node *parse_equality(ListNode** pToks) {
    Node *node = parse_relational(pToks);

    for (;;) {
        if (tok_equal_then_consume(pToks, "==")) {
            node = make_binary(ND_EQ, node, parse_relational(pToks));
            continue;
        }

        if (tok_equal_then_consume(pToks, "!=")) {
            node = make_binary(ND_NE, node, parse_relational(pToks));
            continue;
        }

        return node;
    }
}

static Node* parse_expr(ListNode** pToks)
{
    return parse_equality(pToks);
}

Node* parse(List* tokens)
{
    ListNode* cursor = tokens->front;
    return parse_expr(&cursor);
}