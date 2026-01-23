#include "unity.h"
#include "token.h"
#include "str.h"
#include <stdio.h>
#include <string.h>

int compare_tokens(const Tokens *have, const Token *want) {
    for (size_t i = 0; i < have->len; i++) {
        Token lhs = have->items[i];
        Token rhs = want[i];

        if (lhs.kind != rhs.kind) {
            return false;
        }

        if (lhs.value.items == nullptr && rhs.value.items == nullptr) {
            return true;
        }

        if ((lhs.value.items != nullptr && rhs.value.items == nullptr)
                || (lhs.value.items == nullptr && rhs.value.items != nullptr)) {
            return -1;
        }

        if (lhs.value.len != rhs.value.len) {
            return false;
        }

        if (strcmp(lhs.value.items, rhs.value.items) != 0) {
            return false;
        }
    }

    return true;
}

void setUp(void) {
}

void tearDown(void) {
}

void test_collect(void) {
    String s = string_from_cstr(
            ",.=>{[(&|<-!+}]);*/\n"
            "// This is a comment\n"
            "1234\n"
            "abcd\n"
            "'a'\n"
            "\"Hello, World!\"\n"
    );

    Token want[] = {
        { .kind = TokenKindComma },
        { .kind = TokenKindDot },
        { .kind = TokenKindEqual },
        { .kind = TokenKindGt },
        { .kind = TokenKindLCurly },
        { .kind = TokenKindLBrack },
        { .kind = TokenKindLParen },
        { .kind = TokenKindAmpersand },
        { .kind = TokenKindBar },
        { .kind = TokenKindLt },
        { .kind = TokenKindMinus },
        { .kind = TokenKindExclamation },
        { .kind = TokenKindPlus },
        { .kind = TokenKindRCurly },
        { .kind = TokenKindRBrack },
        { .kind = TokenKindRParen },
        { .kind = TokenKindSemicolon },
        { .kind = TokenKindStar },
        { .kind = TokenKindSlash },
        { .kind = TokenKindNumber, .value = string_from_cstr("1234") },
        { .kind = TokenKindIdent, .value = string_from_cstr("abcd") },
        { .kind = TokenKindChar, .value = string_from_cstr("a") },
        { .kind = TokenKindString, .value = string_from_cstr("Hello, World!") },
        { .kind = TokenKindEof },
    };

    Tokeniser tokeniser = {0};
    tokeniser.src = s;
    int result = token_collect(&tokeniser);

    TEST_ASSERT_MESSAGE(result == 0, "collect failed");
    TEST_ASSERT_MESSAGE(tokeniser.tokens.len == sizeof(want)/sizeof(want[0]), "want.len != have.len");
    TEST_ASSERT_MESSAGE(compare_tokens(&tokeniser.tokens, want), "want != have");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_collect);
    return UNITY_END();
}
