#pragma once

#include "str.h"

typedef struct Writer Writer;
struct Writer {
    void *to;
    void (*append_cstr)(Writer *writer, char *c);
    void (*append_string)(Writer *writer, const String *s);
};

Writer printf_writer;
Writer make_string_writer(String *s);

void writer_append_cstr(Writer *writer, char *s);
void writer_append_string(Writer *writer, const String *s);
