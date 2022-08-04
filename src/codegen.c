#include "minic.h"

#include <assert.h>
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

static void gen_expr(Node const* node)
{
    switch (node->eNodeKind) {
    case ND_NUM:
        printf("  mov $%d, %%rax\n", node->val);
        return;
    case ND_NEG:
        gen_expr(node->rhs);
        printf("  neg %%rax\n");
        return;
    default:
        break;
    }

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
    default:
        break;
    }

    assert(0 && "not implemented");
}

void gen(Node const* node)
{
    gen_expr(node);
}
