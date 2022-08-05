#include "minic.h"

#include <stdio.h>

static uint32_t gen_id()
{
    static uint32_t s_id = 0;
    return ++s_id;
}

static uint32_t gen_expr(Node const* node)
{
    assert(node);

    switch (node->eNodeKind) {
    case ND_NUM: {
        const uint32_t id = gen_id();
        printf("  %%%d = add i32 %d, 0\n", id, node->val);
        return id;
    }
    case ND_NEG: {
        const uint32_t exprId = gen_expr(node->lhs);
        const uint32_t id = gen_id();
        printf("  %%%d = sub i32 0, %%%d\n", id, exprId);
        return id;
    }
    default:
        break;
    }

    const uint32_t lhsId = gen_expr(node->lhs);
    const uint32_t rhsId = gen_expr(node->rhs);
    const uint32_t binId = gen_id();

    switch (node->eNodeKind) {
    case ND_ADD:
        printf("  %%%d = add i32 %%%d, %%%d\n", binId, lhsId, rhsId);
        return binId;
    case ND_SUB:
        printf("  %%%d = sub i32 %%%d, %%%d\n", binId, lhsId, rhsId);
        return binId;
    case ND_MUL:
        printf("  %%%d = mul i32 %%%d, %%%d\n", binId, lhsId, rhsId);
        return binId;
    case ND_DIV:
        printf("  %%%d = sdiv i32 %%%d, %%%d\n", binId, lhsId, rhsId);
        return binId;
    case ND_EQ: {
        printf("  %%%d = icmp eq i32 %%%d, %%%d\n", binId, lhsId, rhsId);
        const uint32_t extId = gen_id();
        printf("  %%%d = zext i1 %%%d to i32\n", extId, binId);
        return extId;
    }
    case ND_NE: {
        printf("  %%%d = icmp ne i32 %%%d, %%%d\n", binId, lhsId, rhsId);
        const uint32_t extId = gen_id();
        printf("  %%%d = zext i1 %%%d to i32\n", extId, binId);
        return extId;
    }
    case ND_GT: {
        printf("  %%%d = icmp sgt i32 %%%d, %%%d\n", binId, lhsId, rhsId);
        const uint32_t extId = gen_id();
        printf("  %%%d = zext i1 %%%d to i32\n", extId, binId);
        return extId;
    }
    case ND_GE: {
        printf("  %%%d = icmp sge i32 %%%d, %%%d\n", binId, lhsId, rhsId);
        const uint32_t extId = gen_id();
        printf("  %%%d = zext i1 %%%d to i32\n", extId, binId);
        return extId;
    }
    case ND_LT: {
        printf("  %%%d = icmp slt i32 %%%d, %%%d\n", binId, lhsId, rhsId);
        const uint32_t extId = gen_id();
        printf("  %%%d = zext i1 %%%d to i32\n", extId, binId);
        return extId;
    }
    case ND_LE: {
        printf("  %%%d = icmp sle i32 %%%d, %%%d\n", binId, lhsId, rhsId);
        const uint32_t extId = gen_id();
        printf("  %%%d = zext i1 %%%d to i32\n", extId, binId);
        return extId;
    }
    default:
        break;
    }

    error_tok(node->tok, "not implemented");
    return -1;
}

static void gen_stmt(Node const* node)
{
    assert(node);

    switch (node->eNodeKind) {
    case ND_RETURN: {
        printf("  ret i32 %%%d\n", gen_expr(node->lhs));
        return;
    }
    case ND_BLOCK:
        for (Node* n = node->body; n; n = n->next) {
            gen_stmt(n);
        }
        return;
    default:
        break;
    }

    error_tok(node->tok, "not implemented");
}

void gen(Function const* prog)
{
    (void)prog;
    printf("define i32 @main() #0 {\n");

    gen_stmt(prog->body);

    printf("}\n");
}
