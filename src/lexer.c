#include "lexer.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Lexer {
    const char* source;
    int sourceLen;
    const char* p;
    int line;
    int col;
} Lexer;

static int is_decimal(char const c)
{
    return '0' <= c && c <= '9';
}

static char lexer_peek(Lexer* lexer)
{
    assert(lexer->p < (lexer->source + lexer->sourceLen));
    return *lexer->p;
}

static char lexer_read(Lexer* lexer)
{
    assert(lexer->p < (lexer->source + lexer->sourceLen));
    char c = *lexer->p++;
    if (c == '\n') {
        ++lexer->line;
        lexer->col = 1;
    } else {
        ++lexer->col;
    }
    return c;
}

static void add_decimal_number(Lexer* lexer, List* list)
{
    Token token;
    {
        token.eKind = TK_NUM;
        token.line = lexer->line;
        token.col = lexer->col;
        token.start = lexer->p;
    }

    while (is_decimal(lexer_peek(lexer))) {
        lexer_read(lexer);
    }

    token.end = lexer->p;
    token.len = token.end - token.start;

    list_push_back(list, token);
}

static void add_one_char_punct(Lexer* lexer, List* list)
{
    Token token;
    {
        token.eKind = TK_PUNCT;
        token.line = lexer->line;
        token.col = lexer->col;
        token.start = lexer->p;
    }

    lexer_read(lexer);
    token.end = lexer->p;
    token.len = 1;

    list_push_back(list, token);
}

static void add_eof(Lexer* lexer, List* list)
{
    Token token;
    {
        token.eKind = TK_EOF;
        token.line = lexer->line;
        token.col = lexer->col;
        token.start = lexer->p;
        token.end = lexer->p;
        token.len = 0;
    }

    list_push_back(list, token);
}

List* lex(const char* source)
{
    List* tokens = list_new();
    Lexer lexer;
    {
        lexer.source = source;
        lexer.sourceLen = strlen(source) + 1;
        lexer.p = source;
        lexer.line = 1;
        lexer.col = 1;
    }

    int c = 0;
    while (c = lexer_peek(&lexer)) {
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
        if (strchr(" \n\t\r", c) != NULL) {
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
            add_decimal_number(&lexer, tokens);
            continue;
        }

        // id or keyword
        // if (is_identifier(c)) {
        //     add_id(tokenList); // keyword could be overriden with #define
        //     continue;
        // }

        // multi-char punct
        // if (try_add_punct(tokenList)) {
        //     continue;
        // }

        // one char punct
        if (strchr("#+-*/%><=&|!?~^()[]{},.:;\\", c) != NULL) {
            add_one_char_punct(&lexer, tokens);
            continue;
        }

        // invalid, can be SHADOWED
        printf("stray '%c' in program\n", c);
        exit(-1);
    }

    add_eof(&lexer, tokens);
    return tokens;
}

const char* tokenkind_to_string(TokenKind eKind)
{
    switch (eKind) {
    case TK_PUNCT:
        return "punct";
    case TK_NUM:
        return "number";
    case TK_EOF:
        return "eof";
    default:
        assert(0 && "not reachable");
        return "";
    }
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

static bool is_identifier(char const c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || '_' == c;
}

static bool is_hex(char const c) {
    return '0' <= c && c <= '9' || 'A' <= c && c <= 'F' || 'a' <= c && c <= 'f';
}

#define T Token*
#define T_NAME(x) Token##x
#define T_FUNC_NAME(x) token_##x
#include "containers/list_impl.h"
#undef T
#undef T_NAME
#undef T_FUNC_NAME

static IntDict *s_keywords = NULL;

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

static char const *s_punctTable[] = {
    "+=", "++", "-=", "--", "->", "*=", "/=", "%=", "==", "!=", "##", ">=",
    ">>=", ">>", "<=", "<<=", "<<", "&&", "||", "&=", "|=", "^=", "..."};

static bool try_add_punct(TokenList *list) {
    Loc *loc = loc_top();
    for (int i = 0; i < ARRAY_SIZE(s_punctTable); ++i) {
        int punctLen = (int)strlen(s_punctTable[i]);
        if (strncmp(loc->p, s_punctTable[i], punctLen) == 0) {
            Token *token = make_empty_token(TK_PUNCT);
            token->len = punctLen;
            memcpy(&token->loc, loc, sizeof(Loc));
            token_list_push_back(list, token);
            shift(punctLen);  // read punct
            return true;
        }
    }

    return false;
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
