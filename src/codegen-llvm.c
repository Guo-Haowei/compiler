#include "minic.h"

#include <stdio.h>

static uint reg_id()
{
    static uint s_id = 0;
    return ++s_id;
}

static uint label_id()
{
    static uint s_id = 0;
    return ++s_id;
}

static void nop()
{
    printf("  %%nop%d = add i1 0, 0 \n", reg_id());
}

static uint gen_expr(Node const* node);
static void gen_stmt(Node const* node);

static uint gen_expr(Node const* node)
{
    assert(node);

    switch (node->eNodeKind) {
    case ND_VAR: {
        assert(node->var);
        // TODO: gen address
        const Obj* var = node->var;
        const uint id = reg_id();
        printf("  %%reg%d = load i32, i32* %%var_%s_%d\n", id, var->name, var->id);
        return id;
    }
    case ND_NUM: {
        const uint id = reg_id();
        printf("  %%reg%d = add i32 %d, 0\n", id, node->val);
        return id;
    }
    case ND_NEG: {
        const uint expr_id = gen_expr(node->lhs);
        const uint id = reg_id();
        printf("  %%reg%d = sub i32 0, %%reg%d\n", id, expr_id);
        return id;
    }
    case ND_ASSIGN: {
        assert(node->lhs->var);
        const Obj* var = node->lhs->var;
        const uint expr_id = gen_expr(node->rhs);
        printf("  store i32 %%reg%d, i32* %%var_%s_%d\n", expr_id, var->name, var->id);
        return expr_id;
    }
    default:
        break;
    }

    const uint lhsId = gen_expr(node->lhs);
    const uint rhsId = gen_expr(node->rhs);
    const uint binId = reg_id();

    switch (node->eNodeKind) {
    case ND_ADD:
        printf("  %%reg%d = add i32 %%reg%d, %%reg%d\n", binId, lhsId, rhsId);
        return binId;
    case ND_SUB:
        printf("  %%reg%d = sub i32 %%reg%d, %%reg%d\n", binId, lhsId, rhsId);
        return binId;
    case ND_MUL:
        printf("  %%reg%d = mul i32 %%reg%d, %%reg%d\n", binId, lhsId, rhsId);
        return binId;
    case ND_DIV:
        printf("  %%reg%d = sdiv i32 %%reg%d, %%reg%d\n", binId, lhsId, rhsId);
        return binId;
    case ND_EQ: {
        printf("  %%reg%d = icmp eq i32 %%reg%d, %%reg%d\n", binId, lhsId, rhsId);
        const uint extId = reg_id();
        printf("  %%reg%d = zext i1 %%reg%d to i32\n", extId, binId);
        return extId;
    }
    case ND_NE: {
        printf("  %%reg%d = icmp ne i32 %%reg%d, %%reg%d\n", binId, lhsId, rhsId);
        const uint extId = reg_id();
        printf("  %%reg%d = zext i1 %%reg%d to i32\n", extId, binId);
        return extId;
    }
    case ND_GT: {
        printf("  %%reg%d = icmp sgt i32 %%reg%d, %%reg%d\n", binId, lhsId, rhsId);
        const uint extId = reg_id();
        printf("  %%reg%d = zext i1 %%reg%d to i32\n", extId, binId);
        return extId;
    }
    case ND_GE: {
        printf("  %%reg%d = icmp sge i32 %%reg%d, %%reg%d\n", binId, lhsId, rhsId);
        const uint extId = reg_id();
        printf("  %%reg%d = zext i1 %%reg%d to i32\n", extId, binId);
        return extId;
    }
    case ND_LT: {
        printf("  %%reg%d = icmp slt i32 %%reg%d, %%reg%d\n", binId, lhsId, rhsId);
        const uint extId = reg_id();
        printf("  %%reg%d = zext i1 %%reg%d to i32\n", extId, binId);
        return extId;
    }
    case ND_LE: {
        printf("  %%reg%d = icmp sle i32 %%reg%d, %%reg%d\n", binId, lhsId, rhsId);
        const uint extId = reg_id();
        printf("  %%reg%d = zext i1 %%reg%d to i32\n", extId, binId);
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
    case ND_IF: {
        const uint id_cond = gen_expr(node->cond);
        const uint id_if = label_id();
        const uint id_else = label_id();
        const uint id_end = label_id();
        const uint id_tmp = reg_id();
        printf("  %%reg%d = trunc i32 %%reg%d to i1\n", id_tmp, id_cond);
        printf("  br i1 %%reg%d, label %%L.if%d, label %%L.els%d\n", id_tmp, id_if, id_else);
        printf("L.if%d:\n", id_if);
        gen_stmt(node->then);
        printf("  br label %%L.end%d\n", id_end);
        printf("L.els%d:\n", id_else);
        if (node->els) {
            gen_stmt(node->els);
        } else {
            nop();
            printf("  br label %%L.end%d\n", id_end);
        }
        printf("L.end%d:\n", id_end);
        nop();
        return;
    }
    case ND_EXPR_STMT:
        gen_expr(node->lhs);
        return;
    case ND_RETURN:
        printf("  store i32 %%reg%d, i32* %%retval\n", gen_expr(node->lhs));
        printf("  br label %%L.return\n");
        return;
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

static void declare_vars(const Function* prog)
{
    for (Obj* var = prog->locals; var; var = var->next) {
        printf("  %%var_%s_%d = alloca i32\n", var->name, var->id);
    }
}

void gen(const Function* prog)
{
    (void)prog;
    printf("define i32 @main() #0 {\n");
    printf("  %%retval = alloca i32\n");
    declare_vars(prog);

    gen_stmt(prog->body);

    printf("  br label %%L.return\n");
    printf("L.return:\n");
    const uint ret_id = reg_id();
    printf("  %%reg%d = load i32, i32* %%retval\n", ret_id);
    printf("  ret i32 %%reg%d\n", ret_id);
    printf("}\n");
}
