#include "minic.h"

#include <stdio.h>
#include <stdlib.h>

#define DEFINE_BASE_TYPE(name, kind, sz, al)                                      \
    static Type s_##name##_type = { .eTypeKind = kind, .size = sz, .align = al }; \
    Type* g_##name##_type = &s_##name##_type;
#include "base_type.inl"
#undef DEFINE_BASE_TYPE

static Type* new_type(TypeKind kind, int size, int align)
{
    Type* ty = calloc(1, sizeof(Type));
    ty->eTypeKind = kind;
    ty->size = size;
    ty->align = align;
    return ty;
}

bool is_integer(Type* type)
{
    switch (type->eTypeKind) {
    case TY_CHAR:
    case TY_INT:
    case TY_SHORT:
    case TY_LONG:
        return true;
    default:
        return false;
    }
}

Type* pointer_to(Type* base)
{
    Type* ty = new_type(TY_PTR, 8, 8);
    ty->eTypeKind = TY_PTR;
    ty->size = 8;
    ty->base = base;
    return ty;
}

Type* array_of(Type* base, int len)
{
    Type* ty = new_type(TY_ARRAY, base->size * len, base->align);
    ty->eTypeKind = TY_ARRAY;
    ty->size = base->size * len;
    ty->base = base;
    ty->arrayLen = len;
    return ty;
}

Type* func_type(Type* retType)
{
    Type* type = calloc(1, sizeof(Type));
    type->eTypeKind = TY_FUNC;
    type->retType = retType;
    return type;
}

Type* copy_type(Type* type)
{
    Type* ret = calloc(1, sizeof(Type));
    *ret = *type;
    return ret;
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
    for (Node* n = node->body; n; n = n->next) {
        add_type(n);
    }
    for (Node* n = node->args; n; n = n->next) {
        add_type(n);
    }
    switch (node->eNodeKind) {
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_NEG:
        node->type = node->lhs->type;
        return;
    case ND_ASSIGN:
        if (node->lhs->type->eTypeKind == TY_ARRAY) {
            error_tok(node->lhs->tok, "not an lvalue");
        }
        node->type = node->lhs->type;
        return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_GT:
    case ND_GE:
    case ND_NUM:
    case ND_FUNCCALL:
        node->type = g_long_type;
        return;
    case ND_VAR:
        node->type = node->var->type;
        return;
    case ND_COMMA:
        node->type = node->rhs->type;
        return;
    case ND_ADDR:
        if (node->lhs->type->eTypeKind == TY_ARRAY) {
            node->type = pointer_to(node->lhs->type->base);
        } else {
            node->type = pointer_to(node->lhs->type);
        }
        return;
    case ND_DEREF:
        if (!node->lhs->type->base) {
            error_tok(node->tok, "invalid pointer dereference, %d", node->lhs->type->eTypeKind);
        }
        if (node->lhs->type->base->eTypeKind == TY_VOID) {
            error_tok(node->tok, "dereferencing a void pointer");
        }
        node->type = node->lhs->type->base;
        return;
    case ND_MEMBER:
        node->type = node->member->type;
        return;
    case ND_IF:
    case ND_FOR:
    case ND_RETURN:
    case ND_BLOCK:
    case ND_EXPR_STMT:
    case ND_INVALID:
    case ND_COUNT:
        return;
    default:
        error("unhandled node type %s\n", node_kind_to_string(node->eNodeKind));
        break;
    }
}