#include "minic.h"

typedef struct {
    List* processed;
    List* unprocessed;
} PreprocState;

bool is_token_equal(const Token* token, const char* symbol)
{
    int len = (int)strlen(symbol);
    if (len != token->len) {
        return false;
    }

    return strncmp(token->start, symbol, token->len) == 0;
}

// static Macro* find_macro(PreprocState* state, const Token* token)
// {
//     Array* macros = &(state->macros);
//     for (int i = 0; i < macros->len; ++i) {
//         Macro* macro = array_at(Macro, macros, i);
//         if (macro->name->len == token->len && strncmp(token->p, macro->name->start, token->len) == 0) {
//             return macro;
//         }
//     }
//     return NULL;
// }

static void preproc2(PreprocState* state);

static bool is_hash(const Token* tok)
{
    return tok->len == 1 && tok->start[0] == '#';
}

static void include(PreprocState* state, Array* preprocLine)
{
    Token* fileNameToken = NULL;
    bool ok = false;
    if (preprocLine->len == 2) {
        fileNameToken = array_at(Token, preprocLine, 1);
        if (fileNameToken->kind == TK_STR) {
            ok = true;
        }
    }

    if (!ok) {
        error_tok(array_at(Token, preprocLine, 0), "#include expects \"FILENAME\"");
    }

    char file[MAX_OSPATH];
    snprintf(file, MAX_OSPATH, "./%s", fileNameToken->sourceInfo->file);
    // remove '/'
    char* p = strrchr(file, '/');
    assert(p);
    *(++p) = '\0';
    strcpy(p, fileNameToken->str);

    Array* rawToks = lex(file);
    // append arr2 to unprocessed list
    for (int i = rawToks->len - 1; i >= 0; --i) {
        const Token* rawTok = array_at(Token, rawToks, i);
        _list_push_front(state->unprocessed, rawTok, sizeof(Token));
    }
}

static Array* getline(PreprocState* state, int line)
{
    Array* preprocLine = array_new(sizeof(Token), 8);
    for (;;) {
        if (list_is_empty(state->unprocessed)) {
            break;
        }
        Token* token = list_front(Token, state->unprocessed);
        if (token->line != line) {
            break;
        }
        _array_push_back(preprocLine, token);
        list_pop_front(state->unprocessed);
    }
    return preprocLine;
}

static void preproc2(PreprocState* state)
{
    for (;;) {
        if (list_is_empty(state->unprocessed)) {
            break;
        }

        const Token* token = list_front(Token, state->unprocessed);
        if (!is_hash(token)) {
            _list_push_back(state->processed, token, sizeof(Token));
            list_pop_front(state->unprocessed);
            continue;
        }

        if (!token->isFirstTok) {
            error_tok(token, "'#' is not the first symbol of the line");
        }

        // ignore line
        const int line = token->line;
        list_pop_front(state->unprocessed); // pop '#'

        Array* preprocLine = getline(state, line);
        if (preprocLine->len == 0) {
            continue;
        }

        Token* directive = array_at(Token, preprocLine, 0);
        if (is_token_equal(directive, "include")) {
            include(state, preprocLine);
            continue;
        }

        for (;;) {
            if (list_is_empty(state->unprocessed)) {
                break;
            }
            const Token* next = list_front(Token, state->unprocessed);
            if (next->line != line) {
                break;
            }
            list_pop_front(state->unprocessed);
        }
    }
}

static void postprocess(List* tokens)
{
    static const char* const s_keywords[] = {
        "auto", "break", "char", "const", "continue", "do", "else", "enum",
        "extern", "for", "if", "int", "long", "return", "short", "sizeof", "static",
        "struct", "typedef", "union", "unsigned", "void", "while"
    };

    for (ListNode* c = tokens->front; c; c = c->next) {
        Token* tok = (Token*)(c + 1);
        for (size_t i = 0; i < ARRAY_COUNTER(s_keywords); ++i) {
            const int len = (int)strlen(s_keywords[i]);
            if (len == tok->len && strncmp(tok->start, s_keywords[i], tok->len) == 0) {
                tok->kind = TK_KEYWORD;
                break;
            }
        }

        if (tok->raw == NULL) {
            tok->raw = strncopy(tok->start, tok->len);
        }
    }

    return;
}

List* preproc(Array* rawTokens)
{
    // copy raw tokens;

    PreprocState state;
    state.processed = list_new();
    state.unprocessed = list_new();

    // copy all tokens to unprocessed
    for (int i = 0; i < rawTokens->len; ++i) {
        const Token* tok = array_at(Token, rawTokens, i);
        _list_push_back(state.unprocessed, tok, sizeof(Token));
    }

    preproc2(&state);

    // convert keywords
    postprocess(state.processed);

    // add eof
    Token* last = list_back(Token, state.processed);
    Token* eof = calloc(1, sizeof(Token));
    eof->kind = TK_EOF;
    eof->line = last->line;
    eof->col = last->col + last->len;
    eof->start = "<eof>";
    eof->raw = "<eof>";
    eof->len = (int)strlen(eof->start);
    eof->sourceInfo = last->sourceInfo;
    _list_push_back(state.processed, eof, sizeof(Token));
    return state.processed;
}
