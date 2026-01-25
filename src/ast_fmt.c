#include <stdio.h>

#include "ast.h"
#include "ast_fmt.h"
#include "array.h"
#include "writer.h"

static void append_cstr(Writer *writer, char *s) {
    writer->append_cstr(writer, s);
}

static void append_string(Writer *writer, const String *s) {
    writer->append_string(writer, s);
}

static void append_indent(Writer *writer, int indent) {
    upto(indent) {
        append_cstr(writer, "    ");
    }
}

void ast_fmt_file(const AstFile *file, Writer *writer) {
    foreach(struct_, &file->structs) {
        ast_fmt_struct(struct_, writer, 0);
    }

    // foreach(function, file->functions) {
    //     ast_fmt_function(function, writer, 0);
    // }
}

void ast_fmt_struct(const AstStruct *struct_, Writer *writer, int indent) {
    append_indent(writer, indent);
    append_cstr(writer, "struct {\n");

    foreach(field, &struct_->fields) {
        ast_fmt_param(field, writer, indent + 1);
        append_cstr(writer, ";\n");
    }

    append_indent(writer, indent);
    append_cstr(writer, "}\n");
}

void ast_fmt_param(const AstParam *param, Writer *writer, int indent) {
    append_indent(writer, indent);
    append_string(writer, &param->name);
    append_cstr(writer, " ");
    append_string(writer, &param->type.name);
}
