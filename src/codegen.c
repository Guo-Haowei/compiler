#include "minic.h"

#include <stdio.h>

static int depth;

static void push()
{
    printf("  push %%rax\n");
    depth++;
}

static void pop(char const* arg)
{
    printf("  pop %s\n", arg);
    depth--;
}

static void gen_cmp_expr(NodeKind eNodeKind) {
    static char const* const s_cmds[] = {
        "sete", "setne",
        "setl", "setle",
        "setg", "setge",
    };

    STATIC_ASSERT(ARRAY_COUNTER(s_cmds) == (ND_GE - ND_EQ + 1));

    int const index = eNodeKind - ND_EQ;
    assertindex(index, ARRAY_COUNTER(s_cmds));

    printf("  cmp %%rdi, %%rax\n");
    printf("  %s %%al\n", s_cmds[index]);
    printf("  movzb %%al, %%rax\n");
}

static void gen_expr(Node const* node)
{
    if (node->eNodeKind == ND_NUM) {
        printf("  mov $%d, %%rax\n", node->val);
        return;
    }

    if (node->eNodeKind == ND_NEG) {
        gen_expr(node->rhs);
        printf("  neg %%rax\n");
        return;
    }

    if (node->isBinary) {
        gen_expr(node->rhs);
        push();
        gen_expr(node->lhs);
        pop("%rdi");

        switch (node->eNodeKind) {
        case ND_ADD:
            printf("  add %%rdi, %%rax\n");
            return;
        case ND_SUB:
            printf("  sub %%rdi, %%rax\n");
            return;
        case ND_MUL:
            printf("  imul %%rdi, %%rax\n");
            return;
        case ND_DIV:
            printf("  cqo\n");
            printf("  idiv %%rdi\n");
            return;
        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
        case ND_GT:
        case ND_GE:
            gen_cmp_expr(node->eNodeKind);
            return;
        default:
            break;
        }
    }

    unreachable();
}

static void gen_stmt(Node const* node)
{
    if (node->eNodeKind == ND_EXPR_STMT) {
        gen_expr(node->rhs);
        return;
    }

    unreachable();
}

void gen(Node const* node)
{
    for (Node const* n = node; n; n = n->next) {
        gen_stmt(n);
        assert(depth == 0);
    }
    printf("  ret\n");
}
