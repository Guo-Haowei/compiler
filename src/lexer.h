#ifndef __LEXER_H__
#define __LEXER_H__
#include "list.h"

typedef enum TokenKind {
    TK_PUNCT, // Punctuators
    TK_NUM,   // Numeric literals
    TK_EOF,   // End-of-file markers
} TokenKind;

typedef struct Token {
    TokenKind eKind;
    int line;
    int col;
    const char* start;
    const char* end;
    int len;
} Token;

List* lex(const char* source);

const char* tokenkind_to_string(TokenKind eKind);

#endif
