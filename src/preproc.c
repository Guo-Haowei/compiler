#include "minic.h"

static void preproc2(Array* inToks, Array* outToks);

static bool is_hash(const Token* tok)
{
    return tok->len == 1 && tok->start[0] == '#';
}

static int skipline(Array* tokens, const Token* hash, int idx)
{
    const int line = hash->line;
    for (; idx < tokens->len; ++idx) {
        const Token* tok = array_at(const Token, tokens, idx);
        if (tok->line != line) {
            break;
        }
    }

    return idx;
}

const Token* get_tok_line(Array* toks, int line, int idx)
{
    if (toks->len <= idx) {
        return NULL;
    }

    const Token* tok = array_at(const Token, toks, idx);
    if (tok->line != line) {
        return NULL;
    }

    return tok;
}

static int handle_directive(Array* toks, const Token* hash, int idx, Array* outToks)
{
    const int line = hash->line;
    for (;;) {
        const Token* tok = array_at(const Token, toks, idx);
        if (tok->line != line) {
            break;
        }

        if (is_token_equal(tok, "include")) {
            const Token* tok2 = get_tok_line(toks, line, idx + 1);
            if (tok2 && tok2->eTokenKind == TK_STR) {
                char file[MAX_OSPATH];
                snprintf(file, MAX_OSPATH, "./%s", tok->sourceInfo->file);
                // remove '/'
                char* p = strrchr(file, '/');
                assert(p);
                *(++p) = '\0';
                strcpy(p, tok2->str);

                Array* inToks = lex(file);
                // append arr2 to list
                preproc2(inToks, outToks);
                return skipline(toks, hash, idx);
            } else {
                error_tok(tok, "#include expects \"FILENAME\"");
            }

        } else {
            // error_tok(tok, "unknown directive \"%.*s\"", tok->len, tok->start);
            return skipline(toks, hash, idx);
        }
    }

    return idx;
}

static void preproc2(Array* inToks, Array* outToks)
{
    for (int idx = 0; idx < inToks->len;) {
        const Token* tok = array_at(const Token, inToks, idx);
        if (!is_hash(tok)) {
            _array_push_back(outToks, tok);
            ++idx;
            continue;
        }

        if (!tok->isFirstTok) {
            error_tok(tok, "'#' is not the first symbol of the line");
        }

        // warn_tok(tok, "extra tokens ignored");
        idx = handle_directive(inToks, tok, idx + 1, outToks);
    }
}

static void convert_kw(Array* toks)
{
    static const char* const s_keywords[] = {
        "auto", "break", "char", "const", "continue", "do", "else", "enum",
        "extern", "for", "if", "int", "long", "return", "short", "sizeof", "static",
        "struct", "typedef", "union", "unsigned", "void", "while"
    };

    for (int i = 0; i < toks->len; ++i) {
        Token* tok = array_at(Token, toks, i);
        for (size_t i = 0; i < ARRAY_COUNTER(s_keywords); ++i) {
            const int len = (int)strlen(s_keywords[i]);
            if (len == tok->len && strncmp(tok->start, s_keywords[i], len) == 0) {
                tok->eTokenKind = TK_KEYWORD;
                break;
            }
        }
    }

    return;
}

Array* preproc(Array* rawtoks)
{
    Array* toks = array_new(sizeof(Token), 128);
    preproc2(rawtoks, toks);

    // convert keywords
    convert_kw(toks);

    // add eof
    // @TODO: array_back()
    const Token* last = array_at(Token, toks, toks->len - 1);
    Token eof;
    memset(&eof, 0, sizeof(Token));
    eof.eTokenKind = TK_EOF;
    eof.line = last->line;
    eof.col = last->col + last->len;
    eof.start = eof.end = (last->end);
    eof.sourceInfo = last->sourceInfo;
    array_push_back(Token, toks, eof);

    return toks;
}
