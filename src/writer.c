#include <stdio.h>

#include "writer.h"

void ast_fmt_printf_write_cstr(Writer *_, char *c) {
    printf("%s", c);
}

void ast_fmt_printf_write_string(Writer *_, const String *s) {
    printf("%.*s", (int)s->len, s->items);
}

Writer ast_fmt_printf_output = {
    .to = nullptr,
    .append_cstr = ast_fmt_printf_write_cstr,
    .append_string = ast_fmt_printf_write_string,
};

void ast_fmt_string_write_cstr(Writer *writer, char *c) {
    string_append_cstr((String*)writer->to, c);
}

void ast_fmt_string_write_string(Writer *writer, const String *s) {
    string_append_string((String*)writer->to, s);
}

Writer make_ast_fmt_string_output(String *s) {
    Writer ast_fmt_string_output = {
        .to = s,
        .append_cstr = ast_fmt_string_write_cstr,
        .append_string = ast_fmt_string_write_string,
    };

    return ast_fmt_string_output;
}
