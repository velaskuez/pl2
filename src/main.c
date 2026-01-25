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
    int res;
    if ((res = token_collect(&tokeniser)) < 0) {
        panic("could not collect tokens: %d", res);
    }

    Parser parser = {0};
    parser.tokens = tokeniser.tokens;
    AstFile file = parse_file(&parser);

    String ast = {0};
    Writer writer = make_ast_fmt_string_output(&ast);
    ast_fmt_file(&file, &writer);

    printf("%.*s", (int)ast.len, ast.items);
}
