#include "cc.h"

#define BASETYPE(NAME, TY, SZ, UNSIGNED)                    \
    static Type s_##NAME##_type = { TY, SZ, SZ, UNSIGNED }; \
    Type* g_##NAME##_type = &s_##NAME##_type;

BASETYPE(void, TY_VOID, 1, false);
BASETYPE(char, TY_CHAR, 1, false);
BASETYPE(short, TY_SHORT, 2, false);
BASETYPE(int, TY_INT, 4, false);
BASETYPE(long, TY_LONG, 8, false);
BASETYPE(uchar, TY_CHAR, 1, true);
BASETYPE(ushort, TY_SHORT, 2, true);
BASETYPE(uint, TY_INT, 4, true);
BASETYPE(ulong, TY_LONG, 8, true);

static Type* new_type(TypeKind kind, int size, int align)
{
    Type* ty = calloc(1, sizeof(Type));
    ty->kind = kind;
    ty->size = size;
    ty->align = align;
    return ty;
}

bool is_integer(Type* type)
{
    switch (type->kind) {
    case TY_CHAR:
    case TY_INT:
    case TY_SHORT:
    case TY_LONG:
    case TY_ENUM:
        return true;
    default:
        return false;
    }
}

Type* pointer_to(Type* base)
{
    Type* ty = new_type(TY_PTR, 8, 8);
    ty->kind = TY_PTR;
    ty->size = 8;
    ty->base = base;
    ty->isUnsigned = true;
    return ty;
}

Type* array_of(Type* base, int len)
{
    Type* ty = new_type(TY_ARRAY, base->size * len, base->align);
    ty->size = base->size * len;
    ty->base = base;
    ty->arrayLen = len;
    return ty;
}

Type* func_type(Type* retType)
{
    Type* type = calloc(1, sizeof(Type));
    type->kind = TY_FUNC;
    type->retType = retType;
    return type;
}

Type* enum_type()
{
    return new_type(TY_ENUM, 4, 4);
}

Type* copy_type(Type* type)
{
    Type* ret = calloc(1, sizeof(Type));
    *ret = *type;
    return ret;
}

Type* struct_type()
{
    return new_type(TY_STRUCT, 0, 1);
}

static Type* get_common_type(Type* ty1, Type* ty2)
{
    if (ty1->base) {
        return pointer_to(ty1->base);
    }

    if (ty1->size < 4) {
        ty1 = g_int_type;
    }
    if (ty2->size < 4) {
        ty2 = g_int_type;
    }

    if (ty1->size != ty2->size) {
        return (ty1->size < ty2->size) ? ty2 : ty1;
    }
    if (ty2->isUnsigned) {
        return ty2;
    }
    return ty1;
}

// For many binary operators, we implicitly promote operands so that
// both operands have the same type. Any integral type smaller than
// int is always promoted to int. If the type of one operand is larger
// than the other's (e.g. "long" vs. "int"), the smaller operand will
// be promoted to match with the other.
//
// This operation is called the "usual arithmetic conversion".
static void usual_arith_conv(Node** lhs, Node** rhs)
{
    Type* type = get_common_type((*lhs)->type, (*rhs)->type);
    *lhs = new_cast(*lhs, type, NULL);
    *rhs = new_cast(*rhs, type, NULL);
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
    if (node->args) {
        for (int i = 0; i < node->args->len; ++i) {
            Node* n = vector_at(Node, node->args, i);
            add_type(n);
        }
    }

    switch (node->kind) {
    case ND_NUM:
    case ND_NOT:
    case ND_LOGOR:
    case ND_LOGAND:
        node->type = g_int_type;
        return;
    case ND_SHL:
    case ND_SHR:
    case ND_BITNOT:
        node->type = node->lhs->type;
        return;
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_MOD:
    case ND_BITAND:
    case ND_BITOR:
    case ND_BITXOR:
        usual_arith_conv(&node->lhs, &node->rhs);
        node->type = node->lhs->type;
        return;
    case ND_NEG: {
        Type* ty = get_common_type(g_int_type, node->lhs->type);
        node->lhs = new_cast(node->lhs, ty, NULL);
        node->type = ty;
        return;
    }
    case ND_ASSIGN: {
        TypeKind kind = node->lhs->type->kind;
        if (kind == TY_ARRAY) {
            error_tok(node->lhs->tok, "not an lvalue");
        }
        if (kind == TY_STRUCT) {
            node->rhs = new_cast(node->rhs, node->lhs->type, NULL);
        }
        node->type = node->lhs->type;
        return;
    }
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
        usual_arith_conv(&node->lhs, &node->rhs);
        node->type = g_int_type;
        return;
    case ND_FUNCCALL:
        node->type = g_long_type;
        return;
    case ND_VAR:
        node->type = node->var->type;
        return;
    case ND_TERNARY:
        if (node->then->type->kind == TY_VOID || node->els->type->kind == TY_VOID) {
            node->type = g_void_type;
        } else {
            usual_arith_conv(&node->then, &node->els);
            node->type = node->then->type;
        }
        return;
    case ND_COMMA:
        node->type = node->rhs->type;
        return;
    case ND_ADDR:
        if (node->lhs->type->kind == TY_ARRAY) {
            node->type = pointer_to(node->lhs->type->base);
        } else {
            node->type = pointer_to(node->lhs->type);
        }
        return;
    case ND_DEREF:
        if (!node->lhs->type->base) {
            error_tok(node->tok, "invalid pointer dereference, %d", node->lhs->type->kind);
        }
        if (node->lhs->type->base->kind == TY_VOID) {
            error_tok(node->tok, "dereferencing a void pointer");
        }
        node->type = node->lhs->type->base;
        return;
    case ND_MEMBER:
        node->type = node->member->type;
        return;
    default:
        break;
    }
}