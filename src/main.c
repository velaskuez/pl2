#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "ast.h"
#include "parse.h"
#include "gen.h"
#include "token.h"
#include "util.h"
#include "str.h"

int main() {
    int fd = open("test.pl2", O_RDONLY);
    if (!fd) {
        panic("could not open test.pl: %s", strerror(errno));
    }

    String src = string_from_file(fd);

    // TODO: something like token_collect_from_file(filename) would be better
    Tokeniser tokeniser = {0};
    tokeniser.filename = string_from_cstr("test.pl2");
    tokeniser.src = src;
    if (token_collect(&tokeniser) < 0) {
        panic("%d:%d: unexpected token", tokeniser.position.line, tokeniser.position.col);
    }

    Parser parser = {0};
    parser.tokens = tokeniser.tokens;
    parser.filename = tokeniser.filename;
    AstFile file = parse_file(&parser);

    // ast_fmt_file(&printf_writer, &file);

    Generator gen = {0};
    gen_init(&gen);

    gen_file(&gen, &file);
}
