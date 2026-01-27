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

void ast_fmt_file(Writer *writer, const AstFile *file) {
    foreach(struct_, &file->structs) {
        ast_fmt_struct(writer, struct_, 0);
        writer_append_cstr(writer, "\n");
    }

    foreach(function, &file->functions) {
        ast_fmt_function(writer, function, 0);
        writer_append_cstr(writer, "\n");
    }
}

void ast_fmt_struct(Writer *writer, const AstStruct *struct_, int indent) {
    writer_append_indent(writer, indent);
    writer_append_cstr(writer, "struct {\n");

    foreach(field, &struct_->fields) {
        ast_fmt_param(writer, field, indent + 1);
        writer_append_cstr(writer, ";\n");
    }

    writer_append_indent(writer, indent);
    writer_append_cstr(writer, "}\n");
}

void ast_fmt_type(Writer *writer, const AstType *type) {
    if (type->pointer) writer_append_cstr(writer, "*");
    writer_append_string(writer, &type->name);
}

void ast_fmt_param(Writer *writer, const AstParam *param, int indent) {
    writer_append_indent(writer, indent);
    writer_append_string(writer, &param->name);
    writer_append_cstr(writer, " ");
    ast_fmt_type(writer, &param->type);
}

void ast_fmt_function(Writer *writer, const AstFunction *function, int indent) {
    writer_append_indent(writer, indent);
    writer_append_cstr(writer, "fn ");
    writer_append_string(writer, &function->name);

    char *sep = "";
    writer_append_cstr(writer, "(");
    foreach(param, &function->params) {
        writer_append_cstr(writer, sep);
        ast_fmt_param(writer, param, 0);
        sep = ", ";
    }
    writer_append_cstr(writer, ")");

    if (function->return_type != nullptr) {
        writer_append_cstr(writer, " ");
        ast_fmt_type(writer, function->return_type);
    }

    writer_append_cstr(writer, " ");
    ast_fmt_block(writer, &function->block, indent + 1);
}

void ast_fmt_block(Writer *writer, const AstBlock *block, int indent) {
    writer_append_cstr(writer, "{\n");
    foreach(statement, &block->statements) {
        ast_fmt_statement(writer, statement, indent);
    }
    writer_append_cstr(writer, "\n}");
}

void ast_fmt_statement(Writer *writer, const AstStatement *statement, int indent) {
    switch (statement->kind) {
        case StatementReturn:
            ast_fmt_statement_return(writer, statement->as.return_, indent);
            break;
        default:
            TODO("implement formatting for other statements");
            break;
    }

    writer_append_cstr(writer, ";");
}

void ast_fmt_statement_return(Writer *writer, const AstExpr *expr, int indent) {
    writer_append_indent(writer, indent);
    writer_append_cstr(writer, "return");

    if (expr != nullptr) {
        writer_append_cstr(writer, " ");
        ast_fmt_expr(writer, expr, 0);
    }
}

void ast_fmt_expr(Writer *writer, const AstExpr *expr, int indent) {
    writer_append_cstr(writer, "TODO: expr");
}
