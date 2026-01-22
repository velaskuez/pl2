#include "stdlib.h"
#include "str.h"

typedef enum {
    // Literals
    TokenKindChar,
    TokenKindNumber,
    TokenKindString,

    // Words
    TokenKindIdent,
    TokenKindKeyword,

    // Symbols
    TokenKindComma,
    TokenKindDot,
    TokenKindEqual,
    TokenKindGt,
    TokenKindLBrace,
    TokenKindLBrack,
    TokenKindLParen,
    TokenKindAmpersand,
    TokenKindBar,
    TokenKindLt,
    TokenKindMinus,
    TokenKindExclamation,
    TokenKindPlus,
    TokenKindRBrace,
    TokenKindRBrack,
    TokenKindRParen,
    TokenKindSemicolon,
    TokenKindSlash,
    TokenKindStar,

    TokenKindEof,
} TokenKind;

typedef struct {
    TokenKind kind;
    String value;
} Token;

typedef struct {
    int line;
    int col;
} Position;

typedef struct {
    size_t len;
    size_t cap;
    Token *items;
} Tokens;

typedef struct {
    String src;
    size_t i;
    Position position;
    Tokens tokens;
} Tokeniser;

int collect(Tokeniser *self);
