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

typedef enum {
    // Literals
    TokenKindChar = 1,
    TokenKindNumber,
    TokenKindString,

    // Words
    TokenKindIdent,

    // Keywords
    KeywordFn,
    KeywordIf,
    KeywordElse,
    KeywordReturn,
    KeywordStruct,
    KeywordWhile,
    KeywordSizeof,
    KeywordNew,

    // Symbols
    TokenKindComma,
    TokenKindDot,
    TokenKindEqual,
    TokenKindGt,
    TokenKindLCurly,
    TokenKindLBrack,
    TokenKindLParen,
    TokenKindAmpersand,
    TokenKindBar,
    TokenKindLt,
    TokenKindMinus,
    TokenKindExclamation,
    TokenKindPlus,
    TokenKindRCurly,
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

int token_collect(Tokeniser *self);
