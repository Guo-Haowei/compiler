#include "minic.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _DEBUG
#undef _DEBUG
#endif

#ifdef _DEBUG
#define DEBUG_ONLY(x) x
#else
#define DEBUG_ONLY(x) ((void)0)
#endif

// @TODO: refactor
//
// Code generator
//

static int depth;

static void push(void)
{
    printf("  push %%rax\n");
    depth++;
}

static void pop(char* arg)
{
    printf("  pop %s\n", arg);
    depth--;
}

static void gen_expr(Node* node)
{
    if (node->eNodeKind == ND_NUM) {
        printf("  mov $%d, %%rax\n", node->val);
        return;
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

    // @TODO: better error
    assert(0 && "invalid expression");
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
        return 1;
    }

    List* tokens = lex(argv[1]);
    DEBUG_ONLY(debug_print_tokens(tokens));

    Node* node = parse(tokens);
    DEBUG_ONLY(debug_print_node(node));

    printf("  .text\n");
    printf("  .globl main\n");
    printf("main:\n");

    gen_expr(node);

    printf("  ret\n");

    list_delete(tokens);
    return 0;
}
