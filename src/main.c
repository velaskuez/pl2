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
#include "check.h"

typedef struct {
    char *input;
    char *output;
    int dependency;
} Options;

typedef struct {
    int position;
    int argc;
    char **argv;
} Args;

char *next_arg(Args *args) {
    if (args->argc == 0 || args->position == args->argc) return nullptr;
    return args->argv[args->position++];
}

Options parse_options(int argc, char** argv) {
    Args args = {0};
    args.argc = argc;
    args.argv = argv;

    next_arg(&args);

    Options options = {0};
    for (;;) {
        char *arg = next_arg(&args);
        if (arg == nullptr) {
            break;
        }

        if (strcmp(arg, "-i") == 0) {
            if (options.input != nullptr) {
                fprintf(stderr, "cannot define -i more than once\n");
                exit(1);
            }

            options.input = next_arg(&args);
            if (options.input == nullptr) {
                fprintf(stderr, "must supply -i with a path\n");
                exit(1);
            }

        } else if (strcmp(arg, "-o") == 0) {
            if (options.output != nullptr) {
                fprintf(stderr, "cannot define -o more than once\n");
                exit(1);
            }

            options.output = next_arg(&args);
            if (options.output == nullptr) {
                fprintf(stderr, "must supply -o with a path\n");
                exit(1);
            }
        } else if (strcmp(arg, "-d") == 0) {
            options.dependency = 1;
        } else {
            fprintf(stderr, "unknown argument: %s\n", arg);
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
