#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "ast.h"
#include "parse.h"
#include "gen.h"
#include "token.h"
#include "util.h"
#include "str.h"
#include "check.h"

typedef struct {
    char *input;
    char *output;
    int dependency;
} Options;

Options parse_options(int argc, char **argv) {
    Options options = {0};

    int opt = 0;
    while ((opt = getopt(argc, argv, "i:o::d")) != -1) {
        switch (opt) {
        case 'i':
            options.input = optarg;
            break;
        case 'o':
            options.output = optarg;
            break;
        case 'd':
            options.dependency = 1;
            break;
        case '?':
        default:
            // TODO: usage()
            fprintf(stderr, "unknown argument: %c\n", opt);
            exit(1);
        }
    }

    return options;
}

void validate_options(Options *options) {
    if (options->input == nullptr) {
        fprintf(stderr, "-i (input) required\n");
        exit(1);
    }

    if (options->output == nullptr) {
        options->output = "a.out";
    }
}

int main(int argc, char** argv) {
    Options options = parse_options(argc, argv);
    validate_options(&options);

    int ifd = open(options.input, O_RDONLY);
    if (!ifd) {
        panic("could not open %s: %s", options.input, strerror(errno));
    }

    String src = string_from_file(ifd);

    int ofd = open(options.output, O_WRONLY | O_CREAT | O_TRUNC, 0660);
    if (!ofd) {
        panic("could not open %s: %s", options.input, strerror(errno));
    }

    // TODO: something like token_collect_from_file(filename) would be better
    Tokeniser tokeniser = {0};
    tokeniser.filename = string_from_cstr(options.input);
    tokeniser.src = src;
    if (token_collect(&tokeniser) < 0) {
        panic("%d:%d: unexpected token", tokeniser.position.line, tokeniser.position.col);
    }

    Parser parser = {0};
    parser.tokens = tokeniser.tokens;
    parser.filename = tokeniser.filename;
    AstFile file = parse_file(&parser);

    // ast_fmt_file(&printf_writer, &file);

    Report report = {0};

    Checker checker = {0};
    check_init(&checker, &report);
    check_file(&checker, &file);

    if (report.errors > 0) return 1;

    Generator gen = {0};
    gen_init(&gen, &report);
    gen.fd = ofd;

    if (!options.dependency) {
        gen.write(&gen, ".entry main");
    }

    gen_file(&gen, &file);
}
