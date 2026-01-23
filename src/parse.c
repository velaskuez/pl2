#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Tokens tokens;
    size_t i;
} Parser;

static TokenKind nth_token(Parser *self, size_t n);
static TokenKind current_token(Parser *self);
static int eat(Parser *self, TokenKind want);
static void expect(Parser *self, TokenKind want);
static int eof(Parser *self);

int parse(Parser *self) {
    while (!eof(self)) {
        if (eat(self, KeywordFn)) {
        } else if (eat(self, KeywordStruct)) {
        } else {
            return -1;
        }
    }

    return 0;
}

TokenKind nth_token(Parser *self, size_t n) {
    if (self->i + n >= self->tokens.len) return TokenKindEof;
    return self->tokens.items[self->i+n].kind;
}

TokenKind current_token(Parser *self) {
    return nth_token(self, 0);
}

int at(Parser *self, TokenKind kind) {
    return current_token(self) == kind;
}

int eat(Parser *self, TokenKind want) {
    if (current_token(self) != want) return false;

    self->i++;
    return true;
}

void expect(Parser *self, TokenKind want) {
    if (eat(self, want)) return;

    // TODO
    fprintf(stderr, "unexpected token: %d, want: %d", current_token(self), want);
    exit(1);
}

int eof(Parser *self) {
    return current_token(self) == TokenKindEof;
}
