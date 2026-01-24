#include <stdio.h>
#include <string.h>

#include "token.h"
#include "array.h"
#include "str.h"

static int isnumber(char c) {
    return c >= '0' && c <= '9';
}

static int iswspace(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

static int isnotquote(char c) {
    return c != '"';
}

static int isalphanumeric(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

static void create_token(Tokeniser *self, TokenKind kind, String value);
static char next_char(Tokeniser *self);
static char peek_char(Tokeniser *self);
static void skip_line(Tokeniser *self);
static void extend_while(Tokeniser *self, String *str, int (condition)(char));
static TokenKind check_keyword(const String *word);

int token_collect(Tokeniser *self) {
    for (;;) {
        char c = next_char(self);
        if (!c) break;
        if (iswspace(c)) continue;

        switch (c) {
        case ',': create_token(self, TokenComma, EMPTY_STRING); continue;
        case '.': create_token(self, TokenDot, EMPTY_STRING); continue;
        case '=': create_token(self, TokenEqual, EMPTY_STRING); continue;
        case '>': create_token(self, TokenGt, EMPTY_STRING); continue;
        case '{': create_token(self, TokenLCurly, EMPTY_STRING); continue;
        case '[': create_token(self, TokenRBrack, EMPTY_STRING); continue;
        case '(': create_token(self, TokenLParen, EMPTY_STRING); continue;
        case '&': create_token(self, TokenAmpersand, EMPTY_STRING); continue;
        case '|': create_token(self, TokenBar, EMPTY_STRING); continue;
        case '<': create_token(self, TokenLt, EMPTY_STRING); continue;
        case '-': create_token(self, TokenMinus, EMPTY_STRING); continue;
        case '!': create_token(self, TokenExclamation, EMPTY_STRING); continue;
        case '+': create_token(self, TokenPlus, EMPTY_STRING); continue;
        case '}': create_token(self, TokenRCurly, EMPTY_STRING); continue;
        case ']': create_token(self, TokenRBrack, EMPTY_STRING); continue;
        case ')': create_token(self, TokenRParen, EMPTY_STRING); continue;
        case ';': create_token(self, TokenSemicolon, EMPTY_STRING); continue;
        case '*': create_token(self, TokenStar, EMPTY_STRING); continue;
        case '/':
            // Skip comments
            c = peek_char(self);
            if (c == '/') {
                skip_line(self);
                continue;
            }

            create_token(self, TokenSlash, EMPTY_STRING);
            continue;
        case '0'...'9':
            String number = {0};
            append(&number, c);
            extend_while(self, &number, isnumber);

            create_token(self, TokenNumber, number);
            continue;
        case '\'':
            String ch = {0};

            c = next_char(self);
            if (c == '\'') {
                return -1;
            }

            append(&ch, c);

            c = next_char(self);
            if (c != '\'') {
                return -1;
            }

            create_token(self, TokenChar, ch);
            continue;

        case '"':
            String str = {0};
            extend_while(self, &str, isnotquote);

            c = next_char(self);
            if (c != '"') {
                return -1;
            }

            create_token(self, TokenString, str);
            continue;
        case 'A'...'Z':
        case 'a'...'z':
            String word = {0};
            extend_while(self, &word, isalphanumeric);

            TokenKind kind = check_keyword(&word);
            if (!kind) kind = TokenIdent;

            create_token(self, kind, word);
            continue;
        default:
            return -1;
        }
    }

    create_token(self, TokenEof, EMPTY_STRING);

    return 0;
}

void create_token(Tokeniser *self, TokenKind kind, String value) {
    Token token = {0};
    token.kind = kind;
    token.value = value;
    token.position = self->position;
    append(&self->tokens, token);
}

char next_char(Tokeniser *self) {
    if (self->i == self->src.len) {
        return 0;
    }

    char c = self->src.items[self->i++];

    self->position.col++;
    if (c == '\n') {
        self->position.line++;
        self->position.col = 0;
    }

    return c;
}

char peek_char(Tokeniser *self) {
    if (self->i == self->src.len) {
        return 0;
    }

    char c = self->src.items[self->i];

    return c;
}

void skip_whitespace(Tokeniser *self) {
    for (;;) {
        char c = peek_char(self);
        if (!c) break;

        if (iswspace(c)) {
            next_char(self);
            continue;
        }

        break;
    }
}

void skip_line(Tokeniser *self) {
    for (;;) {
        char c = next_char(self);
        if (!c) break;
        if (c == '\n') break;
    }
}

void extend_while(Tokeniser *self, String *str, int (condition)(char)) {
    for (;;) {
        char c = peek_char(self);
        if (!c) return;

        if (condition(c)) {
            next_char(self);
            append(str, c);
            continue;
        }

        break;
    }
}

TokenKind check_keyword(const String *word) {
    if (string_cstr_cmp(word, KEYWORD_FN)) return KeywordFn;
    if (string_cstr_cmp(word, KEYWORD_IF)) return KeywordIf;
    if (string_cstr_cmp(word, KEYWORD_ELSE)) return KeywordElse;
    if (string_cstr_cmp(word, KEYWORD_RETURN)) return KeywordReturn;
    if (string_cstr_cmp(word, KEYWORD_STRUCT)) return KeywordStruct;
    if (string_cstr_cmp(word, KEYWORD_WHILE)) return KeywordWhile;
    if (string_cstr_cmp(word, KEYWORD_SIZEOF)) return KeywordSizeof;
    if (string_cstr_cmp(word, KEYWORD_NEW)) return KeywordNew;
    if (string_cstr_cmp(word, KEYWORD_LET)) return KeywordLet;

    return 0;
}
