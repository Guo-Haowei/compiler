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
    tok->p = lexer->p;
    tok->sourceInfo = lexer->sourceInfo;
    tok->isFirstTok = false;
    tok->raw = NULL;
    tok->expandedFrom = NULL;
}

static void add_decimal_number(Lexer* lexer, Array* arr)
{
    Token tok;
    tok.kind = TK_NUM;
    lexer_fill_tok(lexer, &tok);

    while (isdigit(lexer_peek(lexer))) {
        lexer_read(lexer);
    }

    tok.len = (int)(lexer->p - tok.p);

    tok.val = atoll(tok.p);
    array_push_back(Token, arr, tok);
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

static void add_string(Lexer* lexer, Array* arr)
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
    tok.kind = TK_STR;
    lexer_fill_tok(lexer, &tok);
    tok.len = (int)(end - tok.p);

    while (lexer->p != end) {
        lexer_read(lexer);
    }

    tok.type = array_of(g_char_type, len);
    tok.str = buf;

    array_push_back(Token, arr, tok);
}

static void add_identifier_or_keyword(Lexer* lexer, Array* arr)
{
    Token tok;
    tok.kind = TK_IDENT;
    lexer_fill_tok(lexer, &tok);

    lexer_read(lexer);
    while (is_ident2(lexer_peek(lexer))) {
        lexer_read(lexer);
    }
    tok.len = (int)(lexer->p - tok.p);

    array_push_back(Token, arr, tok);
}

static void add_one_char_punct(Lexer* lexer, Array* arr)
{
    Token tok;
    tok.kind = TK_PUNCT;
    lexer_fill_tok(lexer, &tok);

    lexer_read(lexer);
    tok.len = 1;

    array_push_back(Token, arr, tok);
}

static bool try_add_punct(Lexer* lexer, Array* arr)
{
    static const char* s_multi_char_puncts[] = {
        "+=", "++", "-=", "--", "->", "*=", "/=", "%=", "==", "!=", "##", ">=",
        ">>=", ">>", "<=", "<<=", "<<", "&&", "||", "&=", "|=", "^=", "..."
    };

    for (size_t i = 0; i < ARRAY_COUNTER(s_multi_char_puncts); ++i) {
        if (begin_with(lexer->p, s_multi_char_puncts[i])) {
            Token tok;
            tok.kind = TK_PUNCT;
            lexer_fill_tok(lexer, &tok);
            tok.len = (int)strlen(s_multi_char_puncts[i]);
            lexer_shift(lexer, tok.len);
            array_push_back(Token, arr, tok);
            return true;
        }
    }

    return false;
}

static void check_if_bol(Array* toks)
{
    int currentLine = 0;

    for (int idx = 0; idx < toks->len; ++idx) {
        Token* tok = array_at(Token, toks, idx);
        if (tok->kind == TK_EOF) {
            break;
        }

        if (tok->line != currentLine) {
            currentLine = tok->line;
            tok->isFirstTok = true;
        }
    }
}

static Array* lex_source_info(const SourceInfo* sourceInfo)
{
    Array* cached = fcache_get(sourceInfo->file);
    if (cached) {
        return cached;
    }

    Array* tokArray = malloc(sizeof(Array));
    array_init(tokArray, sizeof(Token), 128);

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

        // whitespace
        if (strchr(" \n\t\r", c) != NULL) {
            lexer_read(&lexer);
            continue;
        }

        // string literal
        if (c == '"') {
            add_string(&lexer, tokArray);
            continue;
        }

        // decimal number
        if (isdigit(c)) {
            add_decimal_number(&lexer, tokArray);
            continue;
        }

        // identifier
        if (is_ident1(c)) {
            add_identifier_or_keyword(&lexer, tokArray);
            continue;
        }

        // multi-char punct
        if (try_add_punct(&lexer, tokArray)) {
            continue;
        }

        // one char punct
        if (strchr("=+-*/%()<>{}.,;&[]#", c) != NULL) {
            add_one_char_punct(&lexer, tokArray);
            continue;
        }

        error_lex(&lexer, "stray '%c' in program", c);
    }

    check_if_bol(tokArray);

    fcache_add(sourceInfo->file, tokArray);
    return tokArray;
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
    size_t read = fread(buf, 1, size, fp);
    fclose(fp);

    buf[read] = 0;
    return buf;
}

void lex_file(struct Array* arr, const char* filename);

Array* lex(const char* file)
{
    SourceInfo* sourceInfo = calloc(1, sizeof(SourceInfo));

    path_simplify(file, sourceInfo->file);
    sourceInfo->start = read_file(file);
    sourceInfo->len = (int)strlen(sourceInfo->start);
    sourceInfo->end = sourceInfo->start + sourceInfo->len;

    return lex_source_info(sourceInfo);
}