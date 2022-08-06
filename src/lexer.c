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

static bool is_ident1(char const c)
{
    return is_letter(c) || c == '_';
}

static bool is_ident2(char const c)
{
    return is_ident1(c) || is_decimal(c);
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
    tok.len = (int)(tok.end - tok.start);

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

        // decimal number
        if (is_decimal(c)) {
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
        if (strchr("=+-*/%()<>{},;&", c) != nullptr) {
            add_one_char_punct(&lexer, toks);
            continue;
        }

        error_lex(&lexer, "stray '%c' in program", c);
    }

    add_eof(&lexer, toks);

    return toks;
}
