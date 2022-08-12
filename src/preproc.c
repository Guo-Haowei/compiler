#include "minic.h"

static void check_if_bol(List* toks)
{
    int currentLine = 0;

    for (ListNode* c = toks->front; c; c = c->next) {
        Token* tok = (Token*)(c + 1);
        if (tok->eTokenKind == TK_EOF) {
            break;
        }

        if (tok->line != currentLine) {
            currentLine = tok->line;
            tok->isFirstTok = true;
        }
    }
}

static bool is_hash(Token* tok)
{
    return tok->len == 1 && tok->start[0] == '#';
}

// @TODO: refactor
#define list_node_get(T, n) ((T*)(n + 1))

static List* preproc2(List* toks)
{
    List* newToks = list_new();

    for (ListNode* c = toks->front; c;) {
        Token* tok = list_node_get(Token, c);
        if (!is_hash(tok)) {
            _list_push_back(newToks, tok, sizeof(Token));
            c = c->next;
            continue;
        }

        if (!tok->isFirstTok) {
            error_tok(tok, "'#' is not the first symbol of the line");
        }

        // skip line
        int line = tok->line;
        for (; c; c = c->next) {
            Token* tok2 = list_node_get(Token, c);
            if (tok2->line != line) {
                break;
            }
        }
    }

    return newToks;
}

static void convert_kw(List* toks)
{
    static const char* const s_keywords[] = {
        "auto", "break", "char", "const", "continue", "do", "else", "enum",
        "extern", "for", "if", "int", "long", "return", "short", "sizeof", "static",
        "struct", "typedef", "union", "unsigned", "void", "while"
    };

    for (ListNode* c = toks->front; c; c = c->next) {
        Token* tok = (Token*)(c + 1);

        for (size_t i = 0; i < ARRAY_COUNTER(s_keywords); ++i) {
            const int len = (int)strlen(s_keywords[i]);
            if (len == tok->len && strncmp(tok->start, s_keywords[i], len) == 0) {
                tok->eTokenKind = TK_KEYWORD;
                break;
            }
        }
    }

    return;
}

List* preproc(List* toks)
{
    check_if_bol(toks);

    List* newList = preproc2(toks);
    convert_kw(newList);
    return newList;
}
