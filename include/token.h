#pragma once

#include "stdlib.h"
#include "str.h"

#define KEYWORD_FN "fn"
#define KEYWORD_IF "if"
#define KEYWORD_ELSE "else"
#define KEYWORD_RETURN "return"
#define KEYWORD_STRUCT "struct"
#define KEYWORD_WHILE "while"
#define KEYWORD_SIZEOF "sizeof"
#define KEYWORD_NEW "new"
#define KEYWORD_LET "let"

typedef struct {
    int line;
    int col;
} Position;

typedef enum {
    // Literals
    TokenChar = 1,
    TokenNumber,
    TokenString,

    // Words
    TokenIdent,

    // Keywords
    KeywordFn,
    KeywordIf,
    KeywordElse,
    KeywordReturn,
    KeywordStruct,
    KeywordWhile,
    KeywordSizeof,
    KeywordNew,
    KeywordLet,

    // Symbols
    TokenComma,
    TokenDot,
    TokenEqual,
    TokenGt,
    TokenLCurly,
    TokenLBrack,
    TokenLParen,
    TokenAmpersand,
    TokenBar,
    TokenLt,
    TokenMinus,
    TokenExclamation,
    TokenPlus,
    TokenRCurly,
    TokenRBrack,
    TokenRParen,
    TokenSemicolon,
    TokenSlash,
    TokenStar,

    TokenEof,
} TokenKind;

typedef struct {
    TokenKind kind;
    String value;
    Position position;
} Token;

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

int token_collect(Tokeniser *self);
