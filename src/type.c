#include "minic.h"

#include <stdlib.h>

static Type s_int_type = { TY_INT, nullptr };
Type* g_int_type = &s_int_type;

bool is_integer(Type* ty)
{
    return ty->eTypeKind == TY_INT;
}

Type* pointer_to(Type* base)
{
    Type* ty = calloc(1, sizeof(Type));
    ty->eTypeKind = TY_PTR;
    ty->base = base;
    return ty;
}

void add_type(Node* node)
{
    if (!node || node->type) {
        return;
    }
    add_type(node->lhs);
    add_type(node->rhs);
    add_type(node->cond);
    add_type(node->then);
    add_type(node->els);
    add_type(node->init);
    add_type(node->inc);
    for (Node* n = node->body; n; n = n->next)
        add_type(n);
    switch (node->eNodeKind) {
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_NEG:
    case ND_ASSIGN:
        node->type = node->lhs->type;
        return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_VAR:
    case ND_NUM:
        node->type = g_int_type;
        return;
    case ND_ADDR:
        node->type = pointer_to(node->lhs->type);
        return;
    case ND_DEREF:
        if (node->lhs->type->eTypeKind == TY_PTR) {
            node->type = node->lhs->type->base;
        } else {
            node->type = g_int_type;
        }
        return;
    default:
        return;
    }
}