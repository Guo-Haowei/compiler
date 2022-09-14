#include "cc.h"

typedef struct {
    char input[MAX_OSPATH];
    char output[MAX_OSPATH];
} TranslationUnit;

static char s_includepath[MAX_OSPATH];
static char* s_exename;
static char* s_outname;
static bool s_noexe;
static bool s_irpass;

typedef struct {
    Vector* files;
    Vector* predefined;
} CommandLine;

static Macro* process_predefined(char* line)
{
    assert(*line);
    SourceInfo* sourceInfo = calloc(1, sizeof(SourceInfo));
    strcpy(sourceInfo->file, "<command-line>");
    sourceInfo->start = line;
    sourceInfo->len = (int)strlen(line);
    sourceInfo->end = line + sourceInfo->len;
    Vector* tokens = lex_source_info(sourceInfo);

    assert(tokens->len > 0);

    Macro* macro = calloc(1, sizeof(Macro));
    Token* name = vector_at(Token, tokens, 0);
    if (name->kind != TK_IDENT) {
        error_tok(name, "macro names must be identifiers");
    }
    memcpy(&(macro->token), name, sizeof(Token));
    if (tokens->len == 1) {
        return macro;
    }
    Token* equalSign = vector_at(Token, tokens, 1);
    assert(is_token_equal(equalSign, "="));
    macro->expandTo = list_new();
    for (int i = 2; i < tokens->len; ++i) {
        Token* tok = vector_at(Token, tokens, i);
        _list_push_back(macro->expandTo, tok, sizeof(Token));
    }

    return macro;
}

static void process_args(int argc, char** argv, CommandLine* cmdLine)
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

    cmdLine->files = vector_new(sizeof(TranslationUnit), 4);
    cmdLine->predefined = vector_new(sizeof(Macro*), 4);

    bool hasError = false;
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
            assert(!s_noexe);
            s_noexe = true;
            ++i;
            continue;
        }

        if (strcmp(arg, "-o2") == 0) {
            assert(!s_irpass);
            s_irpass = true;
            ++i;
            continue;
        }

        if (strncmp(arg, "-D", 2) == 0) {
            Macro* macro = process_predefined(arg + 2);
            vector_push_back(Macro*, cmdLine->predefined, macro);
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
        vector_push_back(TranslationUnit, cmdLine->files, unit);
        ++i;
    }

    if (hasError) {
        error("%s: invalid command line\n", s_exename);
    }
}

static void compile_one(TranslationUnit* unit, Vector* predefined)
{
    Vector* rawToks = lex(unit->input);
    List* toks = preproc(rawToks, s_includepath, predefined);
    Obj* prog = parse(toks);

    // only generate IR, but do nothing yet
    if (s_irpass) {
        Vector* funcs = gen_x86_ir(prog);
        gen_x86(funcs, unit->output);
    } else {
        gen(prog, unit->input, unit->output);
    }
}

int main(int argc, char** argv)
{
    CommandLine cmdLine;
    ZERO_MEMORY(cmdLine);
    process_args(argc, argv, &cmdLine);

    if (cmdLine.files->len == 0) {
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

    for (int i = 0; i < cmdLine.files->len; ++i) {
        TranslationUnit* unit = vector_at(TranslationUnit, cmdLine.files, i);
        compile_one(unit, cmdLine.predefined);
    }

    if (!s_noexe) {
        char asms[MAX_OSPATH] = { 0 };
        char* p = asms;
        for (int i = 0; i < cmdLine.files->len; ++i) {
            TranslationUnit* unit = vector_at(TranslationUnit, cmdLine.files, i);
            sprintf(p, " %s", unit->output);
            p = asms + strlen(asms);
        }

        char cmd2[MAX_OSPATH] = { 0 };
        s_outname = s_outname ? s_outname : "a";
        snprintf(cmd2, sizeof(cmd2), "gcc%s -o %s\n", asms, s_outname);
        system(cmd2);

        for (int i = 0; i < cmdLine.files->len; ++i) {
            TranslationUnit* unit = vector_at(TranslationUnit, cmdLine.files, i);
            remove(unit->output);
        }
    }

    return 0;
}
