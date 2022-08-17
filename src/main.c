#include "minic.h"

#include <stdio.h>
#include <string.h>

#if 0
#define DEBUG_ONLY(x) x
#else
#define DEBUG_ONLY(x) ((void)0)
#endif

static const char* s_exename;
static const char* s_input;
static char s_output[MAX_OSPATH];

static void process_args(int argc, const char** argv)
{
    s_exename = argv[0];
    const char* p = strrchr(s_exename, '/');
    if (p) {
        s_exename = p + 1;
    } else {
        p = strrchr(s_exename, '\\');
        if (p) {
            s_exename = p + 1;
        }
    }

    bool hasError = false;
    for (int i = 1; i < argc;) {
        const char* arg = argv[i];
        if (strcmp(arg, "-o") == 0) {
            if (i + 1 >= argc) {
                hasError = true;
                break;
            }

            strncpy(s_output, argv[++i], MAX_OSPATH);
            ++i;
            continue;
        }

        if (strcmp(arg, "-s") == 0) {
            if (i + 1 >= argc) {
                hasError = true;
                break;
            }

            s_input = argv[++i];
            ++i;
            continue;
        }

        hasError = true;
    }

    if (hasError) {
        error("%s: invalid command line\n", s_exename);
    }
}

int main(int argc, const char** argv)
{
    process_args(argc, argv);

    if (s_input == NULL) {
        error("%s: no input files\n", s_exename);
    }

    if (s_output[0] == 0) {
        const size_t size = strlen(s_input);
        strncpy(s_output, s_input, MAX_OSPATH);
        char* p = strrchr(s_output, '.');
        if (!p) {
            p = s_output + size;
        }
        p[1] = 's';
        p[2] = '\0';
    }

    Array* rawToks = lex(s_input);
    DEBUG_ONLY(fprintf(stderr, "*** lex ***\n"));
    Array* toks = preproc(rawToks);
    DEBUG_ONLY(debug_print_tokens(toks));

    DEBUG_ONLY(fprintf(stderr, "*** parse ***\n"));

    Obj* prog = parse(toks);
    DEBUG_ONLY(fprintf(stderr, "*** generate code ***\n"));
    gen(prog, s_input, s_output);

    array_clear(toks);
    free(toks);
    return 0;
}
