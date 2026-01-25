#pragma once

#include "str.h"

typedef struct Writer Writer;
struct Writer {
    void *to;
    void (*append_cstr)(Writer *writer, char *c);
    void (*append_string)(Writer *writer, const String *s);
};
void ast_fmt_printf_write_cstr(Writer *_, char *c);
void ast_fmt_printf_write_string(Writer *_, const String *s);
void ast_fmt_string_write_cstr(Writer *to, char *c);
void ast_fmt_string_write_string(Writer *to, const String *s);

Writer ast_fmt_printf_output;
Writer make_ast_fmt_string_output(String *s);
