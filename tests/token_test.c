#include "unity.h"
#include "token.h"
#include "str.h"
#include <stdio.h>
#include <string.h>

int compare_tokens(const Tokens *have, const Token *want) {
    for (size_t i = 0; i < have->len; i++) {

        Token lhs = have->items[i];
        Token rhs = want[i];

        if (lhs.kind != rhs.kind) return false;

        if (lhs.value.items == nullptr && rhs.value.items == nullptr) return true;

        if ((lhs.value.items != nullptr && rhs.value.items == nullptr)
                || (lhs.value.items == nullptr && rhs.value.items != nullptr)) {
            return false;
        }

        if (lhs.value.len != rhs.value.len) return false;

        if (strcmp(lhs.value.items, rhs.value.items) != 0) return false;

        if (lhs.position.line != rhs.position.line) return false;
        if (lhs.position.col != rhs.position.col) return false;
    }

    return true;
}

void setUp(void) {
}

void tearDown(void) {
}

void test_collect(void) {
    String s = string_from_cstr(
            "struct\n"
            ",.=>{[(&|<-!+}]);*/\n"
            "// This is a comment\n"
            "1234\n"
            "abcd\n"
            "'a'\n"
            "\"Hello, World!\"\n"
            "fn\n"
    );

    Token want[] = {
        { .kind = KeywordStruct, .value = string_from_cstr("struct"),
          .position.col = 0, .position.line = 0 },
        { .kind = TokenComma,
          .position.col = 0, .position.line = 1},
        { .kind = TokenDot,
          .position.col = 1, .position.line = 1},
        { .kind = TokenEqual,
          .position.col = 2, .position.line = 1},
        { .kind = TokenGt,
          .position.col = 3, .position.line = 1},
        { .kind = TokenLCurly,
          .position.col = 4, .position.line = 1},
        { .kind = TokenLBrack,
          .position.col = 5, .position.line = 1},
        { .kind = TokenLParen,
          .position.col = 6, .position.line = 1},
        { .kind = TokenAmpersand,
          .position.col = 7, .position.line = 1},
        { .kind = TokenBar,
          .position.col = 8, .position.line = 1},
        { .kind = TokenLt,
          .position.col = 9, .position.line = 1},
        { .kind = TokenMinus,
          .position.col = 10, .position.line = 1},
        { .kind = TokenExclamation,
          .position.col = 11, .position.line = 1},
        { .kind = TokenPlus,
          .position.col = 12, .position.line = 1},
        { .kind = TokenRCurly,
          .position.col = 13, .position.line = 1},
        { .kind = TokenRBrack,
          .position.col = 14, .position.line = 1},
        { .kind = TokenRParen,
          .position.col = 15, .position.line = 1},
        { .kind = TokenSemicolon,
          .position.col = 16, .position.line = 1},
        { .kind = TokenStar,
          .position.col = 17, .position.line = 1},
        { .kind = TokenSlash,
          .position.col = 18, .position.line = 1},
        { .kind = TokenNumber, .value = string_from_cstr("1234"),
          .position.col = 0, .position.line = 3},
        { .kind = TokenIdent, .value = string_from_cstr("abcd"),
          .position.col = 0, .position.line = 4},
        { .kind = TokenChar, .value = string_from_cstr("a"),
          .position.col = 0, .position.line = 5},
        { .kind = TokenString, .value = string_from_cstr("Hello, World!"),
          .position.col = 0, .position.line = 6},
        { .kind = KeywordFn, .value = string_from_cstr("fn"),
          .position.col = 0, .position.line = 7},
        { .kind = TokenEof,
          .position.col = 0, .position.line = 8},
    };

    Tokeniser tokeniser = {0};
    tokeniser.src = s;
    int result = token_collect(&tokeniser);

    TEST_ASSERT_MESSAGE(result == 0, "collect failed");
    TEST_ASSERT_MESSAGE(tokeniser.tokens.len == sizeof(want)/sizeof(want[0]), "want.len != have.len");
    TEST_ASSERT_MESSAGE(compare_tokens(&tokeniser.tokens, want), "want != have");
    TEST_ASSERT_MESSAGE(tokeniser.position.col == 0, "want != tokeniser.position.col");
    TEST_ASSERT_MESSAGE(tokeniser.position.line == 8, "want != tokeniser.position.line");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_collect);
    return UNITY_END();
}
