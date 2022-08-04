#include "minic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char const* const s_multi_char_puncts[] = {
    "+=", "++", "-=", "--", "->", "*=", "/=", "%=", "==", "!=", "##", ">=",
    ">>=", ">>", "<=", "<<=", "<<", "&&", "||", "&=", "|=", "^=", "..."
};

static bool is_decimal(char const c)
{
    return '0' <= c && c <= '9';
}

static bool is_lowercase(char const c)
{
    return ('a' <= c && c <= 'z');
}

static bool is_uppercase(char const c)
{
    return ('A' <= c && c <= 'Z');
}

static bool is_letter(char const c)
{
    return is_lowercase(c) || is_uppercase(c);
}

static bool begin_with(char const* str, char const* prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

static char lexer_peek(Lexer* lexer)
{
    assert(lexer->p <= lexer->sourceInfo->end);
    return *lexer->p;
}

static char lexer_read(Lexer* lexer)
{
    assert(lexer->p <= lexer->sourceInfo->end);
    char c = *lexer->p++;
    if (c == '\n') {
        ++lexer->line;
        lexer->col = 1;
    } else {
        ++lexer->col;
    }
    return c;
}

static void lexer_shift(Lexer* lexer, int n)
{
    while (n-- > 0) {
        lexer_read(lexer);
    }
}

static void lexer_fill_tok(Lexer const* lexer, Token* tok)
{
    tok->line = lexer->line;
    tok->col = lexer->col;
    tok->start = lexer->p;
    tok->sourceInfo = lexer->sourceInfo;
}

static void add_decimal_number(Lexer* lexer, List* list)
{
    Token tok;
    tok.eTokenKind = TK_NUM;
    lexer_fill_tok(lexer, &tok);

    while (is_decimal(lexer_peek(lexer))) {
        lexer_read(lexer);
    }

    tok.end = lexer->p;
    tok.len = tok.end - tok.start;

    list_push_back(list, tok);
}

static void add_identifier(Lexer* lexer, List* list)
{
    Token tok;
    tok.eTokenKind = TK_IDENT;
    lexer_fill_tok(lexer, &tok);

    lexer_read(lexer);
    // @TODO: implement
    tok.end = lexer->p;
    tok.len = 1;

    list_push_back(list, tok);
}

static void add_one_char_punct(Lexer* lexer, List* list)
{
    Token tok;
    tok.eTokenKind = TK_PUNCT;
    lexer_fill_tok(lexer, &tok);

    lexer_read(lexer);
    tok.end = lexer->p;
    tok.len = 1;

    list_push_back(list, tok);
}

static bool try_add_punct(Lexer* lexer, List* list)
{
    for (size_t i = 0; i < ARRAY_COUNTER(s_multi_char_puncts); ++i) {
        if (begin_with(lexer->p, s_multi_char_puncts[i])) {
            Token tok;
            tok.eTokenKind = TK_PUNCT;
            lexer_fill_tok(lexer, &tok);
            tok.len = strlen(s_multi_char_puncts[i]);
            tok.end = tok.start + tok.len;
            lexer_shift(lexer, tok.len);
            list_push_back(list, tok);
            return true;
        }
    }

    return false;
}

static void add_eof(Lexer* lexer, List* list)
{
    Token tok;
    tok.eTokenKind = TK_EOF;
    lexer_fill_tok(lexer, &tok);
    tok.end = lexer->p;
    tok.len = 0;

    list_push_back(list, tok);
}

List* lex(SourceInfo const* sourceInfo)
{
    List* toks = list_new();
    Lexer lexer;
    lexer.sourceInfo = sourceInfo;
    lexer.p = sourceInfo->start;
    lexer.line = 1;
    lexer.col = 1;

    int c = 0;
    while ((c = lexer_peek(&lexer))) {
        // one line comment
        // if (strncmp(loc->p, "//", 2) == 0) {
        //     skipline();
        //     continue;
        // }

        // comment block
        // if (strncmp(loc->p, "/*", 2) == 0) {
        //     shift(2); // skip /*
        //     // assume comment is always closed
        //     while (strncmp(loc->p, "*/", 2) != 0) {
        //         read();
        //     }
        //     shift(2); // skip */
        //     continue;
        // }

        // whitespace
        if (strchr(" \n\t\r", c) != nullptr) {
            lexer_read(&lexer);
            continue;
        }

        // string
        // if ('"' == c) {
        //     add_string(tokenList);
        //     continue;
        // }

        // char
        // if ('\'' == c) {
        //     add_char(tokenList);
        //     continue;
        // }

        // hex number
        // if (strncmp(loc->p, "0x", 2) == 0) {
        //     add_hex_number(tokenList);
        //     continue;
        // }

        // decimal number
        if (is_decimal(c)) {
            add_decimal_number(&lexer, toks);
            continue;
        }

        if (is_lowercase(c)) {
            add_identifier(&lexer, toks);
            continue;
        }
        // id or keyword
        // if (is_identifier(c)) {
        //     add_id(tokenList); // keyword could be overriden with #define
        //     continue;
        // }

        // multi-char punct
        if (try_add_punct(&lexer, toks)) {
            continue;
        }

        // one char punct
        if (strchr("=+-*/%()<>;", c) != nullptr) {
            add_one_char_punct(&lexer, toks);
            continue;
        }

        error_at_lexer(&lexer, "stray '%c' in program", c);
    }

    add_eof(&lexer, toks);

    return toks;
}

#if 0
static Loc s_locs[MAX_FILES_INCLUDED];
static int s_locCount = 0;

static Loc *loc_push(FileCache *fileCache) {
    assert(s_locCount + 1 < MAX_FILES_INCLUDED);
    Loc *loc = &s_locs[s_locCount];
    loc->fileCache = fileCache;
    loc->ln = 1;
    loc->col = 1;
    loc->p = fileCache->content;
    ++s_locCount;
    return loc;
}

static Loc *loc_top() {
    assert(s_locCount);
    return &s_locs[s_locCount - 1];
}

static void loc_pop() {
    assert(s_locCount);
    Loc *loc = loc_top();
    loc->fileCache = 0;
    loc->ln = 0;
    loc->col = 0;
    loc->p = 0;
    --s_locCount;
}

static char peek() {
    return *loc_top()->p;
}

static char read() {
    Loc *loc = loc_top();
    char c = *loc->p;
    if ('\n' == c) {
        ++loc->ln;
        loc->col = 1;
    } else if ('\r' != c) {
        ++loc->col;
    }

    ++loc->p;
    return c;
}

static void shift(int n) {
    while (n-- > 0) {
        read();
    }
}

static void skipline() {
    Loc *loc = loc_top();
    while (loc->p) {
        read();
        if ('\n' == *loc->p) {
            break;
        }
    }
}

static bool is_hex(char const c) {
    return '0' <= c && c <= '9' || 'A' <= c && c <= 'F' || 'a' <= c && c <= 'f';
}


static Token *make_empty_token(eToken kind) {
    Token *t = CALLOC(sizeof(Token));
    t->kind = kind;
    t->len = 0;
    return t;
}

static void add_id(TokenList *list) {
    Loc *loc = loc_top();
    Token *token = make_empty_token(TK_ID);
    memcpy(&token->loc, loc, sizeof(Loc));

    while (is_identifier(peek()) || is_decimal(peek())) {
        ++token->len;
        read();
    }

    /// TODO: move this after preprocessing
    if (int_dict_nhas(s_keywords, token->loc.p, token->len)) {
        token->kind = TK_KEYWORD;
    }

    token_list_push_back(list, token);
}

static void add_hex_number(TokenList *list) {
    Loc *loc = loc_top();
    Token *token = make_empty_token(TK_NUMBER);
    memcpy(&token->loc, loc, sizeof(Loc));

    shift(2);  // skip 0x;

    // error: invalid suffix "x" on integer constant
    /// TODO: proper error message
    assert(is_hex(peek()));

    while (is_hex(peek())) {
        read();
    }

    token->len = (int)(loc->p - token->loc.p);

    token_list_push_back(list, token);
}

/// TODO: handle escape
static void add_string(TokenList *list) {
    Loc *loc = loc_top();
    Token *token = make_empty_token(TK_STRING);
    memcpy(&token->loc, loc, sizeof(Loc));

    do {
        read();
    } while (peek() != '"' && peek());

    read();  // remove closing "
    ++token->loc.p;
    token->len = (int)(loc->p - token->loc.p - 1);
    token_list_push_back(list, token);
}

/// TODO: handle escape
static void add_char(TokenList *list) {
    Loc *loc = loc_top();
    Token *token = make_empty_token(TK_CHAR);
    memcpy(&token->loc, loc, sizeof(Loc));

    do {
        read();
    } while (peek() != '\'' && peek());

    read();  // remove closing '
    token->len = (int)(loc->p - token->loc.p);
    token_list_push_back(list, token);
}

static void add_strayed(TokenList *list) {
    Loc *loc = loc_top();
    Token *token = make_empty_token(TK_INVALID);
    token->len = 1;
    memcpy(&token->loc, loc, sizeof(Loc));
    token_list_push_back(list, token);
    read();  // read punct
}

static TokenList *tokenize_internal(char const *path, TokenList *tokenList) {
    FileCache *fileCache = filecache_get(path);
    Loc *loc = loc_push(fileCache);

    /// TODO: refactor

    // 2. preprocess
    TokenList *ret = preprocess(tokenList);
    loc_pop();

    return ret;
}

static void init_tables() {
    if (!s_keywords) {
        char const *keywords[] = {
            "auto", "break", "char", "const", "continue", "do", "else", "enum",
            "extern", "for", "if", "int", "return", "sizeof", "static",
            "struct", "typedef", "unsigned", "void", "while"};

        s_keywords = int_dict_make();

        for (int i = 0; i < ARRAY_SIZE(keywords); ++i) {
            int_dict_insert(s_keywords, keywords[i], 1);
        }
    }
}

TokenList *tokenize(char const *path) {
    init_tables();

    TokenList *list = token_list_make();
    list = tokenize_internal(path, list);

    /// TODO: clear macro table after tokenize finished

    return list;
}
#endif
