#include "minic.h"

#define DEBUG_PREPROC 1

typedef struct {
    Token token;
    List* expandTo;

    // function
    bool isFunc;
    List* args;
} Macro;

typedef struct {
    List* processed;
    List* unprocessed;
    Array* macros;
} PreprocState;

bool is_token_equal(const Token* token, const char* symbol)
{
    assert(token);
    // not expanded
    if (!token->expandedFrom) {
        int len = (int)strlen(symbol);
        if (len != token->len) {
            return false;
        }

        return strncmp(token->p, symbol, token->len) == 0;
    }

    assert(token->raw);
    return strcmp(token->raw, symbol) == 0;
}

static Macro* find_macro(PreprocState* state, const Token* token)
{
    Array* macros = state->macros;
    for (int i = 0; i < macros->len; ++i) {
        Macro* macro = array_at(Macro, macros, i);
        const int len = token->raw ? (int)strlen(token->raw) : token->len;
        if (macro->token.len != len) {
            continue;
        }
        const char* start = token->raw ? token->raw : token->p;
        if (strncmp(start, macro->token.p, len) == 0) {
            return macro;
        }
    }
    return NULL;
}

static void preproc2(PreprocState* state);

static bool is_hash(const Token* tok)
{
    return tok->len == 1 && tok->p[0] == '#';
}

typedef struct {
    List* preprocLine;
    ListNode* cursor;
} ProcLineReader;

static Token* peek(ProcLineReader* reader)
{
    if (!reader->cursor) {
        return NULL;
    }

    return (Token*)(reader->cursor + 1);
}

static int is_node_arg(Macro* macro, Token* token)
{
    assert(macro && macro->args);
    int i = 0;
    for (ListNode* argNode = macro->args->front; argNode; argNode = argNode->next) {
        Token* arg = list_node_get(Token, argNode);
        if (!arg->raw) {
            arg->raw = strncopy(arg->p, arg->len);
        }
        if (is_token_equal(token, arg->raw)) {
            return i;
        }
        ++i;
    }
    return -1;
}

static void pop_to(List* tokens, const char* symbol)
{
    for (;;) {
        Token* rest = list_front(Token, tokens);
        if (is_token_equal(rest, symbol)) {
            list_pop_front(tokens);
            break;
        }
        list_pop_front(tokens);
    }
}

static void expand_token(Token* input, Token* original, Token* macroToken)
{
    input->raw = strncopy(input->p, input->len);
    input->p = original->p;
    input->len = original->len;
    input->line = original->line;
    input->col = original->col;
    input->sourceInfo = original->sourceInfo;
    input->expandedFrom = macroToken;
}

static void handle_macro_func(PreprocState* state, Macro* macro, Token* macroName)
{
    TokenReader tr;
    tr.tokens = state->unprocessed;
    tr.cursor = tr.tokens->front;
    tr_expect(&tr, "(");
    Array* args = array_new(sizeof(List), 8);
    Token* tok = tr_peek(&tr);
    if (!is_token_equal(tok, ")")) {
        _array_push_back(args, list_new());
    }

    int numBrackets = 1;
    int numBraces = 0;
    for (;;) {
        Token* tok = tr_read(&tr);
        if (is_token_equal(tok, "(")) {
            ++numBrackets;
        } else if (is_token_equal(tok, ")")) {
            --numBrackets;
            if (numBrackets == 0) {
                break;
            }
        } else if (is_token_equal(tok, "{")) {
            ++numBraces;
            assert(0 && "{ not supported");
        } else if (is_token_equal(tok, "}")) {
            --numBraces;
            assert(0 && "} not supported");
        } else if (is_token_equal(tok, ",")) {
            if (numBrackets == 1) {
                _array_push_back(args, list_new());
                continue;
            }
        }

        List* list = array_back(args);
        assert(list);
        _list_push_back(list, tok, sizeof(Token));
    }

    if (args->len != macro->args->len) {
        error_tok(tr_peek(&tr),
            "macro \"%.*s\" passed %d arguments, but takes %d",
            macro->token.len,
            macro->token.p,
            args->len,
            macro->args->len);
    }

    // pop the tokens
    for (;;) {
        ListNode* front = state->unprocessed->front;
        if (front == tr.cursor) {
            break;
        }
        list_pop_front(state->unprocessed);
    }

    List tmp;
    tmp.back = tmp.front = NULL;
    tmp.len = 0;
    for (ListNode* n = macro->expandTo->front; n;) {
        Token token = *list_node_get(Token, n);
        if (is_token_equal(&token, "##")) {
            assert(0 && "## no supported");
            continue;
        }

        if (is_token_equal(&token, "#")) {
            n = n->next;
            token = *list_node_get(Token, n);
            int idx = is_node_arg(macro, &token);
            assert(idx != -1);
            Token stringToken;
            stringToken.kind = TK_STR;
            stringToken.line = macroName->line;
            stringToken.col = macroName->col;
            stringToken.p = macroName->p;
            stringToken.sourceInfo = macroName->sourceInfo;
            stringToken.isFirstTok = macroName->isFirstTok;
            stringToken.expandedFrom = &macro->token;

            List* argList = array_at(List, args, idx);
            assert(argList);
            const char* start = list_front(Token, argList)->p;
            Token* last = list_back(Token, argList);
            const char* end = last->p + last->len;
            const int len = (int)(end - start);
            stringToken.str = strncopy(start, len);
            stringToken.raw = strncopy(start - 1, len + 2);
            stringToken.raw[0] = '"';
            stringToken.raw[len + 1] = '"';                    // add "
            stringToken.len = len + 2;                         // include quotes
            stringToken.type = array_of(g_char_type, len + 1); // include '\0'

            list_push_back(&tmp, stringToken);
            n = n->next;
            continue;
        }

        int idx = is_node_arg(macro, &token);
        if (idx == -1) {
            expand_token(&token, macroName, &macro->token);
            list_push_back(&tmp, token);
        } else {
            List* argList = array_at(List, args, idx);
            assert(argList);
            for (ListNode* argNode = argList->front; argNode; argNode = argNode->next) {
                Token token = *list_node_get(Token, argNode);
                expand_token(&token, macroName, &macro->token);
                list_push_back(&tmp, token);
            }
        }

        n = n->next;
    }

    // append expanded tokens
    // @TODO: list merge
    tmp.back->next = state->unprocessed->front;
    state->unprocessed->front->prev = tmp.back;
    state->unprocessed->front = tmp.front;
    state->unprocessed->len += tmp.len;
}

static void handle_macro(PreprocState* state, const Token* macroName_)
{
    Token macroName = *macroName_;
    list_pop_front(state->unprocessed);

    if (macroName.kind != TK_IDENT) {
        list_push_back(state->processed, macroName);
        return;
    }

    if (is_token_equal(&macroName, "__LINE__")) {
        macroName.kind = TK_NUM;
        macroName.val = macroName.line;
        char buf[128];
        snprintf(buf, sizeof(buf), "%lld", macroName.val);
        macroName.raw = strncopy(buf, (int)strlen(buf));
        list_push_back(state->processed, macroName);
        return;
    }

    Macro* macro = find_macro(state, &macroName);
    if (!macro) {
        list_push_back(state->processed, macroName);
        return;
    }

    if (macro->expandTo == NULL) {
        return;
    }

    if (macro->isFunc) {
        handle_macro_func(state, macro, &macroName);
        return;
    }

    for (ListNode* n = macro->expandTo->back; n; n = n->prev) {
        Token token = *list_node_get(Token, n);
        expand_token(&token, &macroName, &macro->token);
        list_push_front(state->unprocessed, token);
    }
}

static void define(PreprocState* state, List* preprocLine)
{
    if (list_len(preprocLine) < 2) {
        error_tok(list_front(Token, preprocLine), "no macro name given in #define directive");
    }

    list_pop_front(preprocLine); // pop #define
    const Token* name = list_front(Token, preprocLine);
    if (name->kind != TK_IDENT) {
        error_tok(name, "macro names must be identifier");
    }

    {
        Macro* macro = find_macro(state, name);
        if (macro) {
            assert(0 && "Handle redefinition");
        }
    }

    ProcLineReader reader;
    reader.preprocLine = preprocLine;
    reader.cursor = preprocLine->front->next; // skip name
    Token* tok = peek(&reader);

    Macro macro;
    macro.token = *name;
    macro.isFunc = false;
    list_pop_front(preprocLine); // remove name

    if (tok == NULL) {
        macro.expandTo = NULL;
        array_push_back(Macro, state->macros, macro);
        return;
    }

    if (!is_token_equal(tok, "(")) {
        macro.expandTo = preprocLine;
        array_push_back(Macro, state->macros, macro);
        return;
    }

    // read args
    macro.args = list_new();
    TokenReader tr;
    tr.tokens = preprocLine;
    tr.cursor = preprocLine->front;
    tr_expect(&tr, "(");
    while (!tr_consume(&tr, ")")) {
        if (list_len(macro.args)) {
            tr_expect(&tr, ",");
        }

        Token* arg = tr_read(&tr);
        if (arg->kind != TK_IDENT) {
            assert(0 && "TODO: handle error");
        }
        _list_push_back(macro.args, arg, sizeof(Token));
    }

    pop_to(preprocLine, ")");

    macro.expandTo = preprocLine;
    macro.isFunc = true;
    array_push_back(Macro, state->macros, macro);
}

static void include(PreprocState* state, List* preprocLine)
{
    Token* fileNameToken = NULL;
    bool ok = false;
    // @TODO: support <FILENAME>
    if (list_len(preprocLine) == 2) {
        fileNameToken = (Token*)(preprocLine->front->next + 1);
        if (fileNameToken->kind == TK_STR) {
            ok = true;
        }
    }

    if (!ok) {
        error_tok(list_front(Token, preprocLine), "#include expects \"FILENAME\" or <FILENAME>");
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

static List* getline(PreprocState* state, int line, const char* sourceFile)
{
    List* preprocLine = list_new();
    for (;;) {
        if (list_is_empty(state->unprocessed)) {
            break;
        }
        Token* token = list_front(Token, state->unprocessed);
        if (token->line != line || !streq(token->sourceInfo->file, sourceFile)) {
            break;
        }
        _list_push_back(preprocLine, token, sizeof(Token));
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
            handle_macro(state, token);
            continue;
        }

        if (!token->isFirstTok) {
            error_tok(token, "'#' is not the first symbol of the line");
        }

        // ignore line
        const int line = token->line;
        const char* sourceFile = token->sourceInfo->file;
        list_pop_front(state->unprocessed); // pop '#'

        List* preprocLine = getline(state, line, sourceFile);
        if (list_is_empty(preprocLine)) {
            continue;
        }

        Token* directive = list_front(Token, preprocLine);
        if (is_token_equal(directive, "include")) {
            include(state, preprocLine);
            continue;
        }
        if (is_token_equal(directive, "define")) {
            define(state, preprocLine);
            continue;
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
            if (len == tok->len && strncmp(tok->p, s_keywords[i], tok->len) == 0) {
                tok->kind = TK_KEYWORD;
                break;
            }
        }

        if (tok->raw == NULL) {
            tok->raw = strncopy(tok->p, tok->len);
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
    state.macros = array_new(sizeof(Macro), 8);

    // copy all tokens to unprocessed
    for (int i = 0; i < rawTokens->len; ++i) {
        const Token* tok = array_at(Token, rawTokens, i);
        _list_push_back(state.unprocessed, tok, sizeof(Token));
    }

    preproc2(&state);

    // 1. convert keywords
    // 2. generate raw tokens
    // 3. combine string literals
    postprocess(state.processed);

    // add eof
    Token* last = list_back(Token, state.processed);
    Token* eof = calloc(1, sizeof(Token));
    eof->kind = TK_EOF;
    eof->line = last->line;
    eof->col = last->col + last->len;
    eof->p = "<eof>";
    eof->raw = "<eof>";
    eof->len = (int)strlen(eof->p);
    eof->sourceInfo = last->sourceInfo;
    _list_push_back(state.processed, eof, sizeof(Token));
    return state.processed;
}

void dump_preproc(List* tokens)
{
    int curLine = 1;
    for (ListNode* n = tokens->front; n; n = n->next) {
        Token* tok = list_node_get(Token, n);
        if (tok->line != curLine) {
            printf("%.*s", tok->line - curLine, "\n\n\n\n\n\n");
            curLine = tok->line;
        }
        printf(" %s", tok->raw);
    }
    printf("\n");
}
