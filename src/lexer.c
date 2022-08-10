#include "minic.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool is_ident1(char const c)
{
    return isalpha(c) || c == '_';
}

static bool is_ident2(char const c)
{
    return is_ident1(c) || isdigit(c);
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

static void lexer_skipline(Lexer* lexer)
{
    for (;;) {
        const char c = lexer_peek(lexer);
        if (c == '\n' || c == '\0') {
            break;
        }
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

    while (isdigit(lexer_peek(lexer))) {
        lexer_read(lexer);
    }

    tok.end = lexer->p;
    tok.len = (int)(tok.end - tok.start);

    list_push_back(list, tok);
}

static const char* find_string_end(Lexer* lexer)
{
    const char* p = lexer->p + 1; // skip '"'
    for (; *p != '"'; ++p) {
        if (*p == '\n' || *p == '\0') {
            error_lex(lexer, "unclosed string literal");
        }
        if (*p == '\\') {
            ++p;
        }
    }

    return p + 1;
}

static int from_hex(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if ('a' <= c && c <= 'f') {
        return c - 'a' + 10;
    }
    return c - 'A' + 10;
}

static int read_escaped_char(Lexer* lexer, const char** new_pos, const char* p)
{
    if ('0' <= *p && *p <= '7') {
        // Read an octal number.
        int ocatal = *p++ - '0';
        if ('0' <= *p && *p <= '7') {
            ocatal = (ocatal << 3) + (*p++ - '0');
            if ('0' <= *p && *p <= '7') {
                ocatal = (ocatal << 3) + (*p++ - '0');
            }
        }
        *new_pos = p;
        return ocatal;
    }

    if (*p == 'x') {
        ++p;
        if (!isxdigit(*p)) {
            error_lex(lexer, "invalid hex escape sequence");
        }

        int hex = 0;
        for (; isxdigit(*p); ++p) {
            hex = (hex << 4) + from_hex(*p);
        }
        *new_pos = p;
        return hex;
    }

    *new_pos = p + 1;

    // clang-format off
    static char s_escape[] = {
        '\a', '\b', 'c', 'd', 27, '\f', 'g',
        'h', 'i', 'j', 'k', 'l', 'm', '\n',
        'o', 'p', 'q', '\r', 's', '\t',
        'u', '\v', 'w', 'x', 'y', 'z'
    };
    // clang-format on
    if (islower(*p)) {
        return s_escape[*p - 'a'];
    }

    return *p;
}

static void add_string(Lexer* lexer, List* list)
{
    const char* start = lexer->p;
    const char* end = find_string_end(lexer);
    int maxStringLen = (int)(end - start);
    char* buf = calloc(1, maxStringLen);

    int len = 0;
    for (const char* p = start + 1; *p != '"';) {
        if (*p == '\\') {
            buf[len++] = read_escaped_char(lexer, &p, p + 1);
        } else {
            buf[len++] = *p++;
        }
    }
    assert(len <= maxStringLen);
    ++len;

    Token tok;
    tok.eTokenKind = TK_STR;
    lexer_fill_tok(lexer, &tok);
    tok.end = end;
    tok.len = (int)(tok.end - tok.start);

    while (lexer->p != end) {
        lexer_read(lexer);
    }

    tok.type = array_of(g_char_type, len);
    tok.str = buf;
    list_push_back(list, tok);
}

static void add_identifier_or_keyword(Lexer* lexer, List* list)
{
    static char const* const s_keywords[] = {
        "auto", "break", "char", "const", "continue", "do", "else", "enum",
        "extern", "for", "if", "int", "return", "sizeof", "static",
        "struct", "typedef", "unsigned", "void", "while", nullptr
    };

    Token tok;
    tok.eTokenKind = TK_IDENT;
    lexer_fill_tok(lexer, &tok);

    lexer_read(lexer);
    while (is_ident2(lexer_peek(lexer))) {
        lexer_read(lexer);
    }
    tok.end = lexer->p;
    tok.len = (int)(tok.end - tok.start);

    for (const char* const* p = s_keywords; *p; ++p) {
        const int len = (int)strlen(*p);
        if (len == tok.len && strncmp(tok.start, *p, len) == 0) {
            tok.eTokenKind = TK_KEYWORD;
            break;
        }
    }

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
    static const char* s_multi_char_puncts[] = {
        "+=", "++", "-=", "--", "->", "*=", "/=", "%=", "==", "!=", "##", ">=",
        ">>=", ">>", "<=", "<<=", "<<", "&&", "||", "&=", "|=", "^=", "..."
    };

    for (size_t i = 0; i < ARRAY_COUNTER(s_multi_char_puncts); ++i) {
        if (begin_with(lexer->p, s_multi_char_puncts[i])) {
            Token tok;
            tok.eTokenKind = TK_PUNCT;
            lexer_fill_tok(lexer, &tok);
            tok.len = (int)strlen(s_multi_char_puncts[i]);
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

static List* lex(const SourceInfo* sourceInfo)
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
        if (strncmp(lexer.p, "//", 2) == 0) {
            lexer_skipline(&lexer);
            continue;
        }

        // comment block
        if (strncmp(lexer.p, "/*", 2) == 0) {
            Lexer bak = lexer;
            lexer_shift(&lexer, 2); // skip /*
            // assume comment is always closed
            for (;;) {
                if (lexer.p[0] == '\0' || lexer.p[1] == '\0') {
                    error_lex(&bak, "unterminated comment");
                }

                if (strncmp(lexer.p, "*/", 2) == 0) {
                    break;
                }

                lexer_read(&lexer);
            }
            lexer_shift(&lexer, 2); // skip */
            continue;
        }

        // skip '#' for now
        if (c == '#') {
            lexer_skipline(&lexer);
            continue;
        }

        // whitespace
        if (strchr(" \n\t\r", c) != nullptr) {
            lexer_read(&lexer);
            continue;
        }

        // string literal
        if (c == '"') {
            add_string(&lexer, toks);
            continue;
        }

        // decimal number
        if (isdigit(c)) {
            add_decimal_number(&lexer, toks);
            continue;
        }

        // identifier
        if (is_ident1(c)) {
            add_identifier_or_keyword(&lexer, toks);
            continue;
        }

        // multi-char punct
        if (try_add_punct(&lexer, toks)) {
            continue;
        }

        // one char punct
        if (strchr("=+-*/%()<>{}.,;&[]#", c) != nullptr) {
            add_one_char_punct(&lexer, toks);
            continue;
        }

        error_lex(&lexer, "stray '%c' in program", c);
    }

    add_eof(&lexer, toks);

    return toks;
}

static char* read_file(const char* path)
{
    FILE* fp = fopen(path, "r");
    if (!fp) {
        error("cannot open %s: %s", path, strerror(errno));
    }

    fseek(fp, 0, SEEK_END);
    int size = (int)ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buf = malloc(size + 1);
    int read = fread(buf, 1, size, fp);
    fclose(fp);

    buf[read] = 0;
    return buf;
}

List* lex_file(const char* filename)
{
    SourceInfo* sourceInfo = malloc(sizeof(SourceInfo));
    sourceInfo->file = filename;
    sourceInfo->start = read_file(filename);
    sourceInfo->len = (int)strlen(sourceInfo->start);
    sourceInfo->end = sourceInfo->start + sourceInfo->len;

    return lex(sourceInfo);
}