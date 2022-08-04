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

// Compute the absolute address of a given node.
// It's an error if a given node does not reside in memory.
static void gen_addr(Node const* node)
{
    if (node->eNodeKind == ND_VAR) {
        printf("  lea %d(%%rbp), %%rax\n", node->var->offset);
        return;
    }

    assert(0 && "not an lvalue");
}

static void gen_cmp_expr(NodeKind eNodeKind)
{
    static char const* const s_cmds[] = {
        "sete",
        "setne",
        "setl",
        "setle",
        "setg",
        "setge",
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
    switch (node->eNodeKind) {
    case ND_NUM:
        printf("  mov $%d, %%rax\n", node->val);
        return;
    case ND_NEG:
        gen_expr(node->lhs);
        printf("  neg %%rax\n");
        return;
    case ND_VAR:
        gen_addr(node);
        printf("  mov (%%rax), %%rax\n");
        return;
    case ND_ASSIGN:
        gen_addr(node->lhs);
        push();
        gen_expr(node->rhs);
        pop("%rdi");
        printf("  mov %%rax, (%%rdi)\n");
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

    unreachable();
}

// TODO: refactor
static int if_else_counter()
{
    static int i = 1;
    return i++;
}

static void gen_stmt(Node const* node)
{
    switch (node->eNodeKind) {
    case ND_IF: {
        int const c = if_else_counter();
        gen_expr(node->cond);
        printf("  cmp $0, %%rax\n");
        printf("  je  .L.else.%d\n", c);
        gen_stmt(node->then);
        printf("  jmp .L.end.%d\n", c);
        printf(".L.else.%d:\n", c);
        if (node->els) {
            gen_stmt(node->els);
        }
        printf(".L.end.%d:\n", c);
        return;
    }
    case ND_EXPR_STMT:
        gen_expr(node->lhs);
        return;
    case ND_RETURN:
        gen_expr(node->lhs);
        printf("  jmp .L.return\n");
        return;
    case ND_BLOCK:
        for (Node* n = node->body; n; n = n->next) {
            gen_stmt(n);
        }
        return;
    default:
        break;
    }

    unreachable();
}

// Assign offsets to local variables.
static void assign_lvar_offsets(Function const* prog)
{
    int offset = 0;
    for (Obj* var = prog->locals; var; var = var->next) {
        offset += 8;
        var->offset = -offset;
    }

    // HACK: cast away const qualifier
    ((Function*)prog)->stackSize = align_to(offset, 16);
}

void gen(Function const* prog)
{
    assign_lvar_offsets(prog);

    printf("  .text\n");
    printf("  .globl main\n");
    printf("main:\n");

    // Prologue
    printf("  push %%rbp\n");
    printf("  mov %%rsp, %%rbp\n");
    printf("  sub $%d, %%rsp\n", prog->stackSize);

    gen_stmt(prog->body);
    assert(depth == 0);

    printf(".L.return:\n");
    printf("  mov %%rbp, %%rsp\n");
    printf("  pop %%rbp\n");
    printf("  ret\n");
}
