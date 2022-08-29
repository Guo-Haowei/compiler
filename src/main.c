#include "cc.h"

typedef struct {
    char input[MAX_OSPATH];
    char output[MAX_OSPATH];
} TranslationUnit;

static char s_includepath[MAX_OSPATH];
static char* s_exename;
static char* s_outname;
static bool s_noexe;

static Array* process_args(int argc, char** argv)
{
    s_exename = argv[0];
    char* p = strrchr(s_exename, '/');
    if (p) {
        s_exename = p + 1;
    } else {
        p = strrchr(s_exename, '\\');
        if (p) {
            s_exename = p + 1;
        }
    }

    Array* files = array_new(sizeof(TranslationUnit), 4);

    bool hasError = false;
    bool noS = true;
    for (int i = 1; i < argc;) {
        char* arg = argv[i];

        if (strcmp(arg, "-I") == 0) {
            if (i + 1 >= argc) {
                hasError = true;
                break;
            }

            strncpy(s_includepath, argv[++i], sizeof(s_includepath));
            ++i;
            continue;
        }

        if (strcmp(arg, "-o") == 0) {
            if (i + 1 >= argc) {
                hasError = true;
                break;
            }
            s_outname = argv[++i];
            ++i;
            continue;
        }

        if (strcmp(arg, "-S") == 0) {
            assert(noS);
            noS = true;
            s_noexe = true;
            ++i;
            continue;
        }

        TranslationUnit unit;
        ZERO_MEMORY(unit);
        snprintf(unit.input, MAX_OSPATH, arg);
        char* slash = strrchr(unit.input, '/');
        char* bslash = strrchr(unit.input, '\\');
        char* p = MAX(slash, bslash);
        p = p ? p + 1 : unit.input;
        strncpy(unit.output, p, MAX_OSPATH);
        char* ext = strrchr(unit.output, '.');
        assert(ext);
        ext[1] = 's';
        ext[2] = 0;
        array_push_back(TranslationUnit, files, unit);
        ++i;
    }

    if (hasError) {
        error("%s: invalid command line\n", s_exename);
    }

    return files;
}

static void compile_one(TranslationUnit* unit)
{
    Array* rawToks = lex(unit->input);
    List* toks = preproc(rawToks, s_includepath);
    Obj* prog = parse(toks);

    gen(prog, unit->input, unit->output);
}

int main(int argc, char** argv)
{
    process_args(argc, argv);

    Array* files = process_args(argc, argv);

    if (files->len == 0) {
        error("%s: no input files\n", s_exename);
    }

    if (!s_includepath[0]) {
#if 0
        strncpy(s_includepath, __FILE__, sizeof(s_includepath));
        char* p = strrchr(s_includepath, '/');
        assert(p);
        *p = 0;
        p = strrchr(s_includepath, '/');
        assert(p);
        sprintf(p, "/include/\0");

        printf(s_includepath);
        exit(1);
#endif
    }

    for (int i = 0; i < files->len; ++i) {
        TranslationUnit* unit = array_at(TranslationUnit, files, i);
        compile_one(unit);
    }

    if (!s_noexe) {
        char asms[MAX_OSPATH] = { 0 };
        char* p = asms;
        for (int i = 0; i < files->len; ++i) {
            TranslationUnit* unit = array_at(TranslationUnit, files, i);
            sprintf(p, " %s", unit->output);
            p = asms + strlen(asms);
        }

        char cmd2[MAX_OSPATH] = { 0 };
        s_outname = s_outname ? s_outname : "a";
        snprintf(cmd2, sizeof(cmd2), "gcc%s -o %s\n", asms, s_outname);
        system(cmd2);

        for (int i = 0; i < files->len; ++i) {
            TranslationUnit* unit = array_at(TranslationUnit, files, i);
            remove(unit->output);
        }
    }

    return 0;
}
