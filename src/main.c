#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "ast.h"
#include "ast_fmt.h"
#include "token.h"
#include "util.h"
#include "str.h"
#include "writer.h"

int main() {
    int fd = open("test.pl", O_RDONLY);
    if (!fd) {
        panic("could not open test.pl: %s", strerror(errno));
    }

    String src = string_from_file(fd);
    Tokeniser tokeniser = {0};
    tokeniser.src = src;
    if (token_collect(&tokeniser) < 0) {
        panic("%d:%d: unexpected token", tokeniser.position.line, tokeniser.position.col);
    }

    Parser parser = {0};
    parser.tokens = tokeniser.tokens;
    AstFile file = parse_file(&parser);

    ast_fmt_file(&file, &printf_writer);
}
