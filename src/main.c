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
        sourceInfo.len = (int)strlen(argv[1]);
        sourceInfo.end = sourceInfo.start + sourceInfo.len;
    }

    List* toks = lex(&sourceInfo);
    DEBUG_ONLY(debug_print_tokens(toks));

    Function* prog = parse(toks);
    DEBUG_ONLY(debug_print_node(prog->body));
    DEBUG_ONLY(fprintf(stderr, "*** gen ***\n"));

    gen(prog);

    list_delete(toks);
    return 0;
}
