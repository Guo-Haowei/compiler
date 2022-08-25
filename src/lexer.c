#include "minic.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool is_ident1(char c)
{
    return isalpha(c) || c == '_';
}

static bool is_ident2(char c)
{
    return is_ident1(c) || isdigit(c);
}

static bool begin_with(char* str, char* prefix)
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
        char c = lexer_peek(lexer);
        if (c == '\n' || c == '\0') {
            break;
        }
        lexer_read(lexer);
    }
}

static void lexer_fill_tok(Lexer* lexer, Token* tok)
{
    memset(tok, 0, sizeof(Token));
    tok->line = lexer->line;
    tok->col = lexer->col;
    tok->p = lexer->p;
    tok->sourceInfo = lexer->sourceInfo;
}

static void add_int(Lexer* lexer, Array* arr)
{
    Token tok;
    lexer_fill_tok(lexer, &tok);
    tok.kind = TK_NUM;

    char* p = lexer->p;
    int base = 10;
    if (startswithcase(p, "0x") && isxdigit(p[2])) {
        p += 2;
        base = 16;
    } else if (startswithcase(p, "0b") && (p[2] == '0' || p[2] == '1')) {
        p += 2;
        base = 2;
    } else if (*p == '0') {
        base = 8;
    }

    char* end = NULL;
    uint64_t val = strtoull(p, &end, base);

    // Read U, L or LL suffixes.
    bool l = false;
    bool u = false;
    if (startswithcase(end, "llu") || startswithcase(end, "ull")) {
        end += 3;
        l = u = true;
    } else if (startswithcase(end, "lu") || startswithcase(end, "ul")) {
        end += 2;
        l = u = true;
    } else if (startswithcase(end, "ll")) {
        end += 2;
        l = true;
    } else if (*end == 'L' || *end == 'l') {
        end++;
        l = true;
    } else if (*end == 'U' || *end == 'u') {
        end++;
        u = true;
    }

    if (isalnum(*end)) {
        error_lex(lexer, "invalid number literal");
    }

    Type* ty = NULL;
    if (base == 10) {
        if (l && u) {
            assert(0);
        } else if (l) {
            ty = g_long_type;
        } else if (u) {
            assert(0);
            // ty = (val >> 32) ? ty_ulong : ty_uint;
        } else {
            ty = (val >> 31) ? g_long_type : g_int_type;
        }
    } else {
        if (l && u) {
            // ty = ty_ulong;
            ty = g_long_type;
        } else if (l) {
            // ty = (val >> 63) ? ty_ulong : ty_long;
            ty = (val >> 63) ? g_long_type : g_long_type;
        } else if (u) {
            // ty = (val >> 32) ? ty_ulong : ty_uint;
            ty = (val >> 32) ? g_long_type : g_int_type;
        } else if (val >> 63) {
            // ty = ty_ulong;
            ty = g_long_type;
        } else if (val >> 32) {
            ty = g_long_type;
        } else if (val >> 31) {
            // ty = ty_uint;
            ty = g_int_type;
        } else {
            ty = g_int_type;
        }
    }

    tok.val = val;
    tok.len = (int)(end - tok.p);
    tok.type = ty;

    while (lexer->p != end) {
        lexer_read(lexer);
    }

    array_push_back(Token, arr, tok);
}

static char* find_string_end(Lexer* lexer)
{
    char* p = lexer->p + 1; // skip '"'
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

static int read_escaped_char(Lexer* lexer, char** new_pos, char* p)
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

static void add_char(Lexer* lexer, Array* arr)
{
    Lexer dummy = *lexer;
    char* start = lexer->p;
    lexer_read(lexer);
    char* p = lexer->p;
    if (start[1] == '\0') {
        error_lex(&dummy, "unclosed char literal");
    }

    char c = (*p == '\\') ? read_escaped_char(lexer, &p, p + 1) : *p;
    char* end = strchr(start + 1, '\'');
    if (end == NULL) {
        error_lex(&dummy, "unclosed char literal");
    }
    end = end + 1;

    Token tok;
    ZERO_MEMORY(tok);
    tok.kind = TK_NUM;
    tok.line = lexer->line;
    tok.col = lexer->col;
    tok.p = start;
    tok.len = end - start;
    tok.sourceInfo = lexer->sourceInfo;
    tok.val = c;
    tok.type = g_int_type;

    while (lexer->p != end) {
        lexer_read(lexer);
    }

    array_push_back(Token, arr, tok);
}

static void add_string(Lexer* lexer, Array* arr)
{
    char* start = lexer->p;
    char* end = find_string_end(lexer);
    int maxStringLen = (int)(end - start);
    char* buf = calloc(1, maxStringLen);

    int len = 0;
    for (char* p = start + 1; *p != '"';) {
        if (*p == '\\') {
            buf[len++] = read_escaped_char(lexer, &p, p + 1);
        } else {
            buf[len++] = *p++;
        }
    }
    assert(len <= maxStringLen);
    ++len;

    Token tok;
    lexer_fill_tok(lexer, &tok);
    tok.kind = TK_STR;
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
    lexer_fill_tok(lexer, &tok);
    tok.kind = TK_IDENT;

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
    lexer_fill_tok(lexer, &tok);
    tok.kind = TK_PUNCT;

    lexer_read(lexer);
    tok.len = 1;

    array_push_back(Token, arr, tok);
}

static char s_multi_char_puncts[][4] = {
    "+=", "++", "-=", "--", "->", "*=", "/=", "%=", "==", "!=", "##", ">=",
    ">>=", ">>", "<=", "<<=", "<<", "&&", "||", "&=", "|=", "^=", "..."
};

static bool try_add_punct(Lexer* lexer, Array* arr)
{

    for (size_t i = 0; i < ARRAY_COUNTER(s_multi_char_puncts); ++i) {
        if (begin_with(lexer->p, s_multi_char_puncts[i])) {
            Token tok;
            lexer_fill_tok(lexer, &tok);
            tok.kind = TK_PUNCT;
            tok.len = (int)strlen(s_multi_char_puncts[i]);
            lexer_shift(lexer, tok.len);
            array_push_back(Token, arr, tok);
            return true;
        }
    }

    return false;
}

static void postprocess(Array* toks)
{
    int currentLine = 0;

    for (int idx = 0; idx < toks->len; ++idx) {
        Token* tok = array_at(Token, toks, idx);
        assert(tok->kind != TK_EOF);

        // copy string
        tok->raw = strncopy(tok->p, tok->len);

        // check if begin of line
        if (tok->line != currentLine) {
            currentLine = tok->line;
            tok->isFirstTok = true;
        }
    }
}

static Array* lex_source_info(SourceInfo* sourceInfo)
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

        // char literal
        if (c == '\'') {
            add_char(&lexer, tokArray);
            continue;
        }

        // string literal
        if (c == '"') {
            add_string(&lexer, tokArray);
            continue;
        }

        // decimal number
        if (isdigit(c)) {
            add_int(&lexer, tokArray);
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
        if (strchr("=+-*/%()<>{}.,;&[]#!~&|^:?\\", c) != NULL) {
            add_one_char_punct(&lexer, tokArray);
            continue;
        }

        error_lex(&lexer, "stray '%c' in program", c);
    }

    postprocess(tokArray);

    fcache_add(sourceInfo->file, tokArray);
    return tokArray;
}

static char* read_file(char* path)
{
    FILE* fp = fopen(path, "r");
    if (!fp) {
        error("cannot open %s: %s\n", path, strerror(errno));
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

void lex_file(Array* arr, char* filename);

Array* lex(char* file)
{
    SourceInfo* sourceInfo = calloc(1, sizeof(SourceInfo));

    path_simplify(file, sourceInfo->file);
    sourceInfo->start = read_file(file);
    sourceInfo->len = (int)strlen(sourceInfo->start);
    sourceInfo->end = sourceInfo->start + sourceInfo->len;

    return lex_source_info(sourceInfo);
}