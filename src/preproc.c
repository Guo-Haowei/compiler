/// lexing and preprocessing
/// pass1: lex source to an array of tokens, cache for reuse
/// pass2: preprocess, such as #include, #if...
/// pass3: cleanup and check if keywords
#include "cc.h"

typedef struct {
    bool active;
} CondIf;

typedef struct {
    Token token;
    List* expandTo;

    // function
    List* args;
    bool isFunc;
    bool isVararg;
} Macro;

typedef struct {
    List* conditions;
    List* processed;
    List* unprocessed;
    Dict* macros;
    char* includepath;
} PreprocState;

static Token* new_eof()
{
    Token* eof = calloc(1, sizeof(Token));
    eof->kind = TK_EOF;
    eof->raw = eof->p = "<eof>";
    eof->len = (int)strlen(eof->raw);
    return eof;
}

bool is_token_equal(Token* token, char* symbol)
{
    assert(token && token->raw);
    return streq(token->raw, symbol);
}

static bool is_active(PreprocState* state)
{
    if (list_is_empty(state->conditions)) {
        return true;
    }

    return list_back(CondIf, state->conditions)->active;
}

static Macro* find_macro(PreprocState* state, Token* token)
{
    assert(token->raw);
    return dict_get(state->macros, token->raw);
}

static void preproc2(PreprocState* state);

static bool is_hash(Token* tok)
{
    return tok->len == 1 && tok->raw[0] == '#';
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
        assert(arg->raw);
        if (is_token_equal(token, arg->raw)) {
            return i;
        }
        ++i;
    }
    return -1;
}

static void pop_to(List* tokens, ListNode* node)
{
    for (;;) {
        ListNode* front = tokens->front;
        if (front == node) {
            break;
        }
        list_pop_front(tokens);
    }
}

static void expand_token(Token* input, Token* original, Token* macroToken)
{
    input->len = original->len;
    input->line = original->line;
    input->col = original->col;
    input->sourceInfo = original->sourceInfo;
    input->expandedFrom = macroToken;
}

static void handle_macro_func(PreprocState* state, Macro* macro, Token* macroName)
{
    TokenReader tr;
    tr.cursor = state->unprocessed->front;
    tr_expect(&tr, "(");
    Array* args = array_new(sizeof(List), 8);
    Token* tok = tr_peek(&tr);
    if (!is_token_equal(tok, ")")) {
        _array_push_back(args, list_new());
    }

    int numBrackets = 1;
    for (;;) {
        Token* tok = tr_read(&tr);
        if (is_token_equal(tok, "(")) {
            ++numBrackets;
        } else if (is_token_equal(tok, ")")) {
            --numBrackets;
            if (numBrackets == 0) {
                break;
            }
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

    pop_to(state->unprocessed, tr.cursor);

    if (args->len != macro->args->len && !macro->isVararg) {
        error_tok(tr_peek(&tr), "macro \"%s\" passed %d arguments, but takes %d", macro->token.raw, args->len, macro->args->len);
    }

    List* tmp = list_new();
    for (ListNode* n = macro->expandTo->front; n;) {
        Token token = *list_node_get(Token, n);
        if (is_token_equal(&token, "##")) {
            n = n->next;
            Token* paste = list_node_get(Token, n);
            int idx = is_node_arg(macro, paste);
            if (idx != -1) {
                List* argList = array_at(List, args, idx);
                assert(argList->len == 1);
                assert(tmp->len);
                paste = list_front(Token, argList);
            }
            Token* back = list_back(Token, tmp);
            int backLen = (int)strlen(back->raw);
            char* str = calloc(1, backLen + paste->len + 1);
            sprintf(str, "%s%s", back->raw, paste->raw);
            back->raw = str;
            n = n->next;
            continue;
        }

        if (is_token_equal(&token, "__VA_ARGS__")) {
            bool isFirst = true;
            for (int i = macro->args->len; i < args->len; ++i) {
                if (!isFirst) {
                    Token comma;
                    ZERO_MEMORY(comma);
                    comma.raw = ",";
                    comma.kind = TK_PUNCT;
                    expand_token(&comma, macroName, &macro->token);
                    list_push_back(tmp, comma);
                }
                isFirst = false;

                List* argList = array_at(List, args, i);
                assert(argList);
                for (ListNode* argNode = argList->front; argNode; argNode = argNode->next) {
                    Token token = *list_node_get(Token, argNode);
                    expand_token(&token, macroName, &macro->token);
                    list_push_back(tmp, token);
                }
            }
            n = n->next;
            continue;
        }

        if (is_token_equal(&token, "#")) {
            n = n->next;
            token = *list_node_get(Token, n);
            int idx = is_node_arg(macro, &token);
            assert(idx != -1);
            Token stringToken;
            ZERO_MEMORY(stringToken);
            stringToken.kind = TK_STR;
            stringToken.line = macroName->line;
            stringToken.col = macroName->col;
            stringToken.sourceInfo = macroName->sourceInfo;
            stringToken.isFirstTok = macroName->isFirstTok;
            stringToken.expandedFrom = &macro->token;

            List* argList = array_at(List, args, idx);
            assert(argList);
            char* start = list_front(Token, argList)->p;
            Token* last = list_back(Token, argList);
            char* end = last->p + last->len;
            int len = (int)(end - start);
            stringToken.str = strncopy(start, len);
            stringToken.raw = strncopy(start - 1, len + 2);
            stringToken.raw[0] = '"';
            stringToken.raw[len + 1] = '"';                    // add "
            stringToken.len = len + 2;                         // include quotes
            stringToken.type = array_of(g_char_type, len + 1); // include '\0'

            list_push_back(tmp, stringToken);
            n = n->next;
            continue;
        }

        int idx = is_node_arg(macro, &token);
        if (idx == -1) {
            expand_token(&token, macroName, &macro->token);
            list_push_back(tmp, token);
        } else {
            List* argList = array_at(List, args, idx);
            assert(argList);
            for (ListNode* argNode = argList->front; argNode; argNode = argNode->next) {
                Token token = *list_node_get(Token, argNode);
                expand_token(&token, macroName, &macro->token);
                list_push_back(tmp, token);
            }
        }

        n = n->next;
    }

    List* newList = list_append(tmp, state->unprocessed);
    free(state->unprocessed);
    state->unprocessed = newList;
}

static void handle_macro(PreprocState* state, Token* macroName_)
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
        macroName.type = g_int_type;
        list_push_back(state->processed, macroName);
        return;
    }

    if (is_token_equal(&macroName, "__FILE__")) {
        macroName.kind = TK_STR;
        macroName.str = macroName.sourceInfo->file;
        char buf[MAX_OSPATH];
        snprintf(buf, MAX_OSPATH, "\"%s\"", macroName.str);
        macroName.raw = strdup(buf);
        macroName.type = array_of(g_char_type, strlen(macroName.sourceInfo->file) + 1);
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
        if (list_len(macro->expandTo)) {
            handle_macro_func(state, macro, &macroName);
        }
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
    Token* name = list_front(Token, preprocLine);
    if (name->kind != TK_IDENT) {
        error_tok(name, "macro names must be identifiers");
    }

    Macro* found = find_macro(state, name);
    if (found) {
        //info_tok(&(found->token), "this is the location of the previous definition");
        error_tok(name, "'%s' redefined", name->raw);
    }

    ProcLineReader reader;
    reader.preprocLine = preprocLine;
    reader.cursor = preprocLine->front->next; // skip name
    Token* tok = peek(&reader);

    Macro* macro = calloc(1, sizeof(Macro));
    macro->token = *name;
    macro->isFunc = false;
    list_pop_front(preprocLine); // remove name

    if (tok == NULL) {
        macro->expandTo = NULL;
        bool ok = dict_try_add(state->macros, macro->token.raw, macro);
        assert(ok);
        return;
    }

    // check if there's a space, if so
    bool isBracket = is_token_equal(tok, "(");
    bool noSpace = macro->token.p + macro->token.len == tok->p;
    if (!(isBracket && noSpace)) {
        macro->expandTo = preprocLine;
        bool ok = dict_try_add(state->macros, macro->token.raw, macro);
        assert(ok);
        return;
    }

    // read args
    macro->args = list_new();
    TokenReader tr;
    tr.cursor = preprocLine->front;
    tr_expect(&tr, "(");
    while (!tr_consume(&tr, ")")) {
        if (list_len(macro->args)) {
            tr_expect(&tr, ",");
        }

        Token* arg = tr_read(&tr);
        if (is_token_equal(arg, "...")) {
            tr_expect(&tr, ")");
            macro->isVararg = true;
            break;
        }

        if (arg->kind != TK_IDENT) {
            error_tok(arg, "expected parameter name, found \"%s\"", arg->raw);
        }
        _list_push_back(macro->args, arg, sizeof(Token));
    }

    pop_to(preprocLine, tr.cursor);

    macro->expandTo = preprocLine;
    macro->isFunc = true;
    bool ok = dict_try_add(state->macros, macro->token.raw, macro);
    assert(ok);
}

static void undef(PreprocState* state, List* preprocLine)
{
    if (list_len(preprocLine) < 2) {
        error_tok(list_front(Token, preprocLine), "no macro name given in #undef directive");
    }

    list_pop_front(preprocLine); // pop #undef
    Token* name = list_front(Token, preprocLine);
    if (name->kind != TK_IDENT) {
        error_tok(name, "macro names must be identifiers");
    }

    Macro* found = find_macro(state, name);
    if (!found) {
        return;
    }

    bool ok = dict_erase(state->macros, name->raw);
    assert(ok);
}

static void if_defined(PreprocState* state, List* preprocLine)
{
    Token* start = list_front(Token, preprocLine);
    if (list_len(preprocLine) < 2) {
        error_tok(start, "no macro name given in #%s directive", start->raw);
    }

    bool isIfdef = is_token_equal(start, "ifdef");
    list_pop_front(preprocLine); // pop #ifdef or #ifndef

    Token* name = list_front(Token, preprocLine);
    if (name->kind != TK_IDENT) {
        error_tok(name, "macro names must be identifiers");
    }

    Macro* found = find_macro(state, name);
    bool active = false;
    if (is_active(state)) {
        active = (found && isIfdef) || (!found && !isIfdef);
    }

    CondIf cond = { active };
    list_push_back(state->conditions, cond);
}

static void if_clause(PreprocState* state, List* preprocLine)
{
    Token* start = list_front(Token, preprocLine);
    if (list_len(preprocLine) < 2) {
        error_tok(start, "#if with no expression");
    }

    list_pop_front(preprocLine); // pop #if

    // parse contexpr
    _list_push_back(preprocLine, new_eof(), sizeof(Token)); // add eof for guarding
    ParserState parserState;
    ZERO_MEMORY(parserState);
    parserState.reader.cursor = preprocLine->front;

    int val = parse_constexpr(&parserState);
    bool active = is_active(state) && (val != 0);

    CondIf cond = { active };
    list_push_back(state->conditions, cond);
}

static void include(PreprocState* state, List* preprocLine)
{
    char file[MAX_OSPATH];

    bool ok = false;

    if (list_len(preprocLine) >= 2) {
        Token* start = (Token*)(preprocLine->front->next + 1);
        if (start->kind == TK_STR) {
            snprintf(file, MAX_OSPATH, "./%s", start->sourceInfo->file);
            // remove '/'
            char* p = strrchr(file, '/');
            assert(p);
            *(++p) = '\0';
            strcpy(p, start->str);
            ok = true;
        }

        if (start->len == 1 && start->raw[0] == '<') {
            char* end = strchr(start->p, '>');
            if (end) {
                bool pathOk = true;
                for (char* p = start->p; p != end; ++p) {
                    if (*p == '\n') {
                        pathOk = false;
                        break;
                    }
                }

                if (pathOk) {
                    int pathLen = end - start->p - 1;
                    snprintf(file, MAX_OSPATH, "./%s/%.*s", state->includepath, pathLen, start->p + 1);
                    ok = true;
                }
            }
        }
    }

    if (!ok) {
        error_tok(list_front(Token, preprocLine), "#include expects \"FILENAME\" or <FILENAME>");
    }

    // include_ok:
    Array* rawToks = lex(file);
    // append arr2 to unprocessed list
    for (int i = rawToks->len - 1; i >= 0; --i) {
        Token* rawTok = array_at(Token, rawToks, i);
        _list_push_front(state->unprocessed, rawTok, sizeof(Token));
    }

    return;
}

static List* getline(PreprocState* state, int line, char* sourceFile)
{
    List* preprocLine = list_new();
    for (;;) {
        if (list_is_empty(state->unprocessed)) {
            break;
        }
        Token* token = list_front(Token, state->unprocessed);
        if (!streq(token->sourceInfo->file, sourceFile)) {
            break;
        }

        if (token->len == 1 && token->raw[0] == '\\') {
            list_pop_front(state->unprocessed);
            assert(list_len(state->unprocessed));
            token = list_front(Token, state->unprocessed);
            line = token->line;
            continue;
        }

        if (token->line != line) {
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

        Token* token = list_front(Token, state->unprocessed);
        if (!is_hash(token)) {
            if (is_active(state)) {
                handle_macro(state, token);
            } else {
                list_pop_front(state->unprocessed);
            }
            continue;
        }

        if (!token->isFirstTok && !is_active(state)) {
            error_tok(token, "'#' is not the first symbol of the line");
        }

        // ignore line
        int line = token->line;
        char* sourceFile = token->sourceInfo->file;
        list_pop_front(state->unprocessed); // pop '#'

        List* preprocLine = getline(state, line, sourceFile);
        if (list_is_empty(preprocLine)) {
            continue;
        }

        Token* directive = list_front(Token, preprocLine);
        if (is_token_equal(directive, "endif")) {
            if (list_is_empty(state->conditions)) {
                error_tok(directive, "#endif without #if");
            }
            list_pop_back(state->conditions);
            continue;
        }

        if (is_token_equal(directive, "else")) {
            if (list_is_empty(state->conditions)) {
                error_tok(directive, "#else without #if");
            }
            ListNode* parent = state->conditions->back->prev;
            bool isParentActive = !parent || list_node_get(CondIf, parent)->active;
            if (isParentActive) {
                CondIf* cond = list_back(CondIf, state->conditions);
                cond->active = !cond->active;
            }
            continue;
        }

        if (is_token_equal(directive, "ifdef") || is_token_equal(directive, "ifndef")) {
            if_defined(state, preprocLine);
            continue;
        }

        if (is_token_equal(directive, "if")) {
            if_clause(state, preprocLine);
            continue;
        }

        if (!is_active(state)) {
            continue;
        }
        if (is_token_equal(directive, "include")) {
            include(state, preprocLine);
            continue;
        }
        if (is_token_equal(directive, "define")) {
            define(state, preprocLine);
            continue;
        }
        if (is_token_equal(directive, "undef")) {
            undef(state, preprocLine);
            continue;
        }

        error_tok(directive, "invalid preprocessing directive #%s", directive->raw);
    }
}

static void postprocess(List* tokens)
{
    static char* s_keywords[] = {
        "auto", "break", "case", "char", "const", "continue", "default", "do", "else", "enum", "extern", "for", "go", "if", "int", "long", "return", "short", "signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "while"
    };
    STATIC_ASSERT(ARRAY_COUNTER(s_keywords) == 28);

    for (ListNode* c = tokens->front; c; c = c->next) {
        Token* tok = (Token*)(c + 1);
        assert(tok->raw);
        for (size_t i = 0; i < ARRAY_COUNTER(s_keywords); ++i) {
            int len = (int)strlen(s_keywords[i]);
            if (len == tok->len && streq(tok->raw, s_keywords[i])) {
                tok->kind = TK_KEYWORD;
                break;
            }
        }
    }

    return;
}

List* preproc(Array* toks, char* includepath)
{
    assert(includepath);
    // @TODO: add predefined macro

    PreprocState state;
    state.processed = list_new();
    state.unprocessed = list_new();
    state.conditions = list_new();
    state.macros = dict_new();
    state.includepath = includepath;

    // copy all tokens to unprocessed
    for (int i = 0; i < toks->len; ++i) {
        Token* tok = array_at(Token, toks, i);
        _list_push_back(state.unprocessed, tok, sizeof(Token));
    }

    preproc2(&state);
    postprocess(state.processed);

    // add eof
    Token* eof = new_eof();
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

Token* tr_peek_n(TokenReader* reader, int n)
{
    ListNode* c = reader->cursor;
    if (n >= 0) {
        for (int i = 0; i < n; ++i) {
            if (!c) {
                return NULL;
            }
            c = c->next;
        }
    } else {
        for (int i = 0; i < -n; ++i) {
            if (!c) {
                return NULL;
            }
            c = c->prev;
        }
    }
    return c ? (Token*)(c + 1) : NULL;
}

Token* tr_peek(TokenReader* reader)
{
    return tr_peek_n(reader, 0);
}

Token* tr_read(TokenReader* reader)
{
    Token* ret = tr_peek(reader);
    assert(ret);
    reader->cursor = reader->cursor->next;
    return ret;
}

bool tr_equal(TokenReader* reader, char* symbol)
{
    return is_token_equal(tr_peek(reader), symbol);
}

bool tr_consume(TokenReader* reader, char* symbol)
{
    if (tr_equal(reader, symbol)) {
        assert(reader->cursor);
        reader->cursor = reader->cursor->next;
        return true;
    }

    return false;
}

void tr_expect(TokenReader* reader, char* symbol)
{
    Token* token = tr_peek(reader);
    if (is_token_equal(token, symbol)) {
        reader->cursor = reader->cursor->next;
        return;
    }

    error_tok(token, "expected '%s', got '%s'", symbol, token->raw);
}
