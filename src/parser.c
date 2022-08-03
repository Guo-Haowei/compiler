#include "minic.h"

#include <assert.h>
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
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static void tok_consume(ListNode** pTokens)
{
    assert(pTokens && *pTokens);
    *pTokens = (*pTokens)->next;
}

static bool tok_equal_then_consume(ListNode** pTokens, const char* expect)
{
    assert(pTokens && *pTokens);

    Token const* token = (Token const*)(*pTokens + 1);
    int const expectLen = strlen(expect);
    if (expectLen != token->len) {
        return false;
    }

    bool const isEqual = strncmp(token->start, expect, expectLen) == 0;
    if (isEqual) {
        *pTokens = (*pTokens)->next;
        return true;
    }

    return false;
}

static void tok_expect(ListNode** pTokens, char const* expect)
{
    assert(pTokens && *pTokens);
    Token const* token = (Token const*)(*pTokens + 1);
    int const expectLen = strlen(expect);
    if (expectLen != token->len || (strncmp(token->start, expect, token->len) != 0)) {
        error_at_token(token, "expected '%s'", expect);
    }

    *pTokens = (*pTokens)->next;
}

static Node* parse_add(ListNode** pTokens);

// primary = "(" expr ")" | num
static Node* parse_primary(ListNode** pTokens)
{
    if (tok_equal_then_consume(pTokens, "(")) {
        Node* node = parse_add(pTokens);
        tok_expect(pTokens, ")"); // consume ')'
        return node;
    }

    Token const* token = (Token const*)(*pTokens + 1);
    if (token->eTokenKind == TK_NUM) {
        Node* node = make_num(token_as_int(token));
        tok_consume(pTokens);
        return node;
    }

    error_at_token(token, "expected expression");
    return nullptr;
}

// mul = primary ("*" primary | "/" primary)*
static Node* parse_mul(ListNode** pTokens)
{
    Node* node = parse_primary(pTokens);

    for (;;) {
        if (tok_equal_then_consume(pTokens, "*")) {
            node = make_binary(ND_MUL, node, parse_primary(pTokens));
            continue;
        }

        if (tok_equal_then_consume(pTokens, "/")) {
            node = make_binary(ND_DIV, node, parse_primary(pTokens));
            continue;
        }

        if (tok_equal_then_consume(pTokens, "%")) {
            node = make_binary(ND_REM, node, parse_primary(pTokens));
            continue;
        }

        return node;
    }
}

// expr = mul ("+" mul | "-" mul)*
static Node* parse_add(ListNode** pTokens)
{
    Node* node = parse_mul(pTokens);

    for (;;) {
        if (tok_equal_then_consume(pTokens, "+")) {
            node = make_binary(ND_ADD, node, parse_mul(pTokens));
            continue;
        }

        if (tok_equal_then_consume(pTokens, "-")) {
            node = make_binary(ND_SUB, node, parse_mul(pTokens));
            continue;
        }

        return node;
    }
}

Node* parse(List* tokens)
{
    ListNode* cursor = tokens->front;
    return parse_add(&cursor);
}