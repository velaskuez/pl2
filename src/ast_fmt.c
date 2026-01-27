#include <stdio.h>

#include "ast.h"
#include "ast_fmt.h"
#include "array.h"
#include "writer.h"
#include "util.h"

static void writer_append_indent(Writer *writer, int indent) {
    upto(indent) {
        writer_append_cstr(writer, "    ");
    }
}

void ast_fmt_file(const AstFile *file, Writer *writer) {
    foreach(struct_, &file->structs) {
        ast_fmt_struct(struct_, writer, 0);
        writer_append_cstr(writer, "\n");
    }

    foreach(function, &file->functions) {
        ast_fmt_function(function, writer, 0);
        writer_append_cstr(writer, "\n");
    }
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

void ast_fmt_type(const AstType *type, Writer *writer) {
    if (type->pointer) writer_append_cstr(writer, "*");
    writer_append_string(writer, &type->name);
}

void ast_fmt_param(const AstParam *param, Writer *writer, int indent) {
    writer_append_indent(writer, indent);
    writer_append_string(writer, &param->name);
    writer_append_cstr(writer, " ");
    ast_fmt_type(&param->type, writer);
}

void ast_fmt_function(const AstFunction *function, Writer *writer, int indent) {
    writer_append_indent(writer, indent);
    writer_append_cstr(writer, "fn ");
    writer_append_string(writer, &function->name);

    char *sep = "";
    writer_append_cstr(writer, "(");
    foreach(param, &function->params) {
        writer_append_cstr(writer, sep);
        ast_fmt_param(param, writer, 0);
        sep = ", ";
    }
    writer_append_cstr(writer, ")");

    if (function->return_type != nullptr) {
        writer_append_cstr(writer, " ");
        ast_fmt_type(function->return_type, writer);
    }

    writer_append_cstr(writer, " ");
    ast_fmt_block(&function->block, writer, indent + 1);
}

void ast_fmt_block(const AstBlock *block, Writer *writer, int indent) {
    writer_append_cstr(writer, "{\n");
    foreach(statement, &block->statements) {
        ast_fmt_statement(statement, writer, indent);
    }
    writer_append_cstr(writer, "\n}");
}

void ast_fmt_statement(const AstStatement *statement, Writer *writer, int indent) {
    switch (statement->kind) {
        case StatementReturn:
            ast_fmt_statement_return(statement->as.return_, writer, indent);
            break;
        default:
            TODO("implement formatting for other statements");
            break;
    }

    writer_append_cstr(writer, ";");
}

void ast_fmt_statement_return(const AstExpr *expr, Writer *writer, int indent) {
    writer_append_indent(writer, indent);
    writer_append_cstr(writer, "return");

    if (expr != nullptr) {
        writer_append_cstr(writer, " ");
        ast_fmt_expr(expr, writer, 0);
    }
}

void ast_fmt_expr(const AstExpr *expr, Writer *writer, int indent) {
    writer_append_cstr(writer, "TODO: expr");
}
