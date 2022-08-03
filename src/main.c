#include "minic.h"

#include <stdio.h>
#include <string.h>

#if 0
#define DEBUG_ONLY(x) x
#else
#define DEBUG_ONLY(x) ((void)0)
#endif

int main(int argc, char** argv)
{
    if (argc != 2) {
        fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
        return 1;
    }

    SourceInfo sourceInfo;
    {
        sourceInfo.file = "<unknown>";
        sourceInfo.start = argv[1];
        sourceInfo.len = strlen(argv[1]);
        sourceInfo.end = sourceInfo.start + sourceInfo.len;
    }

    List* toks = lex(&sourceInfo);
    DEBUG_ONLY(debug_print_tokens(toks));

    Node* node = parse(toks);
    DEBUG_ONLY(debug_print_node(node));

    printf("  .text\n");
    printf("  .globl main\n");
    printf("main:\n");

    gen(node);

    printf("  ret\n");

    list_delete(toks);
    return 0;
}
