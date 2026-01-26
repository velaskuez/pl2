#include <stdio.h>

#include "writer.h"

void writer_printf_write_cstr(Writer *_, char *c) {
    printf("%s", c);
}

void writer_printf_write_string(Writer *_, const String *s) {
    printf("%.*s", (int)s->len, s->items);
}

Writer printf_writer = {
    .to = nullptr,
    .append_cstr = writer_printf_write_cstr,
    .append_string = writer_printf_write_string,
};

void writer_string_write_cstr(Writer *writer, char *c) {
    string_append_cstr((String*)writer->to, c);
}

void writer_string_write_string(Writer *writer, const String *s) {
    string_append_string((String*)writer->to, s);
}

Writer make_string_writer(String *s) {
    Writer ast_fmt_string_output = {
        .to = s,
        .append_cstr = writer_string_write_cstr,
        .append_string = writer_string_write_string,
    };

    return ast_fmt_string_output;
}

void writer_append_cstr(Writer *writer, char *s) {
    writer->append_cstr(writer, s);
}

void writer_append_string(Writer *writer, const String *s) {
    writer->append_string(writer, s);
}
