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

    List* toks = lex_file(argv[1]);
    DEBUG_ONLY(fprintf(stderr, "*** lex ***\n"));
    toks = preproc(toks);
    DEBUG_ONLY(debug_print_tokens(toks));

    DEBUG_ONLY(fprintf(stderr, "*** parse ***\n"));
    Obj* prog = parse(toks);
    DEBUG_ONLY(fprintf(stderr, "*** generate code ***\n"));

    gen(prog, argv[1]);

    list_delete(toks);
    return 0;
}
