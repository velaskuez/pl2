#include <stdio.h>
#include <string.h>

#include "tokeniser.h"
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
static int is_keyword(const String *word);

static char* keywords[] = {
    "func",
    "if",
    "else",
    "return",
    "struct",
    "while",
    "sizeof",
    "new",
};

#define EMPTY_STRING (String){0}

int collect(Tokeniser *self) {
    for (;;) {
        char c = next_char(self);
        if (!c) break;
        if (iswspace(c)) continue;

        switch (c) {
        case ',': create_token(self, TokenKindComma, EMPTY_STRING); continue;
        case '.': create_token(self, TokenKindDot, EMPTY_STRING); continue;
        case '=': create_token(self, TokenKindEqual, EMPTY_STRING); continue;
        case '>': create_token(self, TokenKindGt, EMPTY_STRING); continue;
        case '{': create_token(self, TokenKindLBrace, EMPTY_STRING); continue;
        case '[': create_token(self, TokenKindRBrack, EMPTY_STRING); continue;
        case '(': create_token(self, TokenKindLParen, EMPTY_STRING); continue;
        case '&': create_token(self, TokenKindAmpersand, EMPTY_STRING); continue;
        case '|': create_token(self, TokenKindBar, EMPTY_STRING); continue;
        case '<': create_token(self, TokenKindLt, EMPTY_STRING); continue;
        case '-': create_token(self, TokenKindMinus, EMPTY_STRING); continue;
        case '!': create_token(self, TokenKindExclamation, EMPTY_STRING); continue;
        case '+': create_token(self, TokenKindPlus, EMPTY_STRING); continue;
        case '}': create_token(self, TokenKindRBrace, EMPTY_STRING); continue;
        case ']': create_token(self, TokenKindRBrack, EMPTY_STRING); continue;
        case ')': create_token(self, TokenKindRParen, EMPTY_STRING); continue;
        case ';': create_token(self, TokenKindSemicolon, EMPTY_STRING); continue;
        case '*': create_token(self, TokenKindStar, EMPTY_STRING); continue;
        case '/':
            // Skip comments
            c = peek_char(self);
            if (c == '/') {
                skip_line(self);
                continue;
            }

            create_token(self, TokenKindSlash, EMPTY_STRING);
            continue;
        case '0'...'9':
            String number = {0};
            append(&number, c);
            extend_while(self, &number, isnumber);

            create_token(self, TokenKindNumber, number);
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

            create_token(self, TokenKindChar, ch);
            continue;

        case '"':
            String str = {0};
            extend_while(self, &str, isnotquote);

            c = next_char(self);
            if (c != '"') {
                return -1;
            }

            create_token(self, TokenKindString, str);
            continue;
        case 'A'...'Z':
        case 'a'...'z':
            String word = {0};
            extend_while(self, &word, isalphanumeric);

            TokenKind kind = is_keyword(&word) ? TokenKindKeyword : TokenKindIdent;
            create_token(self, kind, word);
            continue;
        default:
            return -1;
        }
    }

    return 0;
}

void create_token(Tokeniser *self, TokenKind kind, String value) {
    Token token = {0};
    token.kind = kind;
    token.value = value;
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

int is_keyword(const String *word) {
    for (size_t i = 0; i < sizeof(keywords)/sizeof(keywords[0]); i++) {
        char *keyword = keywords[i];
        if (strlen(keyword) != word->len) continue;
        if (strcmp(keyword, word->items) == 0) return true;
    }

    return false;
}
