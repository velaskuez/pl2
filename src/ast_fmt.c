#include <stdio.h>

#include "ast.h"
#include "ast_fmt.h"
#include "array.h"
#include "writer.h"

static void writer_append_indent(Writer *writer, int indent) {
    upto(indent) {
        writer_append_cstr(writer, "    ");
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
    writer_append_indent(writer, indent);
    writer_append_cstr(writer, "struct {\n");

    foreach(field, &struct_->fields) {
        ast_fmt_param(field, writer, indent + 1);
        writer_append_cstr(writer, ";\n");
    }

    writer_append_indent(writer, indent);
    writer_append_cstr(writer, "}\n");
}

void ast_fmt_param(const AstParam *param, Writer *writer, int indent) {
    writer_append_indent(writer, indent);
    writer_append_string(writer, &param->name);
    writer_append_cstr(writer, " ");
    writer_append_string(writer, &param->type.name);
}
