#include "lexer.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static int expect_number(ListNode* tokenNode)
{
    assert(tokenNode);
    const Token* token = (Token*)(tokenNode + 1);
    if (token->eKind != TK_NUM) {
        // @TODO:
        // error("expect a number");
    }

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%.*s", token->len, token->start);
    return atoi(buffer);
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
        return 1;
    }

    List* tokens = lex(argv[1]);

#if 0
    for (ListNode* n = tokens->front; n; n = n->next) {
        Token* token = (Token*)(n + 1);
        fprintf(stderr, "<%s> '%.*s'\n", tokenkind_to_string(token->eKind), token->len, token->start);
    }
#endif

    printf("  .text\n");
    printf("  .globl main\n");
    printf("main:\n");

    ListNode* cursor = tokens->front;
    printf("  mov $%d, %%rax\n", expect_number(cursor));
    cursor = cursor->next;

    for (;; cursor = cursor->next) {
        const Token* token = (Token*)(cursor + 1);
        if (token->eKind == TK_EOF) {
            break;
        }

        if (*token->start == '+') {
            cursor = cursor->next;
            printf("  add $%d, %%rax\n", expect_number(cursor));
            continue;
        }

        if (*token->start == '-') {
            cursor = cursor->next;
            printf("  sub $%d, %%rax\n", expect_number(cursor));
            continue;
        }

        return 1;
    }

    printf("  ret\n");

    list_delete(tokens);
    return 0;
}
