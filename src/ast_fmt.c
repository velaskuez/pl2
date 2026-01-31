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
    char *sep = "";
    foreach(statement, &block->statements) {
        writer_append_cstr(writer, sep);
        ast_fmt_statement(writer, statement, indent);
        sep = "\n";
    }
    writer_append_cstr(writer, "\n}");
}

void ast_fmt_statement(Writer *writer, const AstStatement *statement, int indent) {
    switch (statement->kind) {
    case StatementReturn:
        ast_fmt_statement_return(writer, statement->as.return_, indent);
        break;
    case StatementAssign:
        writer_append_cstr(writer, "TODO assign");
        break;
    case StatementLet:
        writer_append_cstr(writer, "TODO let");
        break;
    case StatementExpr:
        ast_fmt_expr(writer, &statement->as.expr, indent);
        break;
    case StatementIf:
        writer_append_cstr(writer, "TODO if");
        break;
    case StatementWhile:
        writer_append_cstr(writer, "TODO while");
        break;
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
    writer_append_indent(writer, indent);

    switch (expr->kind) {
    case ExprBinaryOp:
        ast_fmt_binary_op(writer, &expr->as.binary_op);
        break;
    case ExprUnaryOp:
        ast_fmt_unary_op(writer, &expr->as.unary_op);
        break;
    case ExprValue:
        ast_fmt_value(writer, &expr->as.value);
        break;
    case ExprIdent:
        writer_append_string(writer, &expr->as.ident);
        break;
    case ExprCompoundIdent:
        ast_fmt_compound_ident(writer, &expr->as.compound_ident);
        break;
    case ExprCall:
        ast_fmt_call(writer, &expr->as.call);
        break;
    }
}

void ast_fmt_value(Writer *writer, const AstValue *value) {
    switch (value->kind) {
    case ValueString:
        writer_append_cstr(writer, "\"");
        writer_append_string(writer, &value->as.string);
        writer_append_cstr(writer, "\"");
        break;
    case ValueChar:
        char *str = nullptr;
        asprintf(&str, "'%c'", (wchar_t)value->as.char_);
        writer_append_cstr(writer, str);
        free(str);
        break;
    case ValueNumber:
        char *num = nullptr;
        asprintf(&num, "%ld", value->as.number);
        writer_append_cstr(writer, num);
        free(num);
        break;
    }
}

void ast_fmt_binary_op(Writer *writer, const AstBinaryOp *binary_op) {
    if (binary_op->op != BinaryOpIndex) writer_append_cstr(writer, "(");

    ast_fmt_expr(writer, binary_op->left, 0);

    if (binary_op->op == BinaryOpIndex) {
        writer_append_cstr(writer, "[");
    } else {
        writer_append_cstr(writer, " ");
        writer_append_cstr(writer, binary_op_str[binary_op->op]);
        writer_append_cstr(writer, " ");
    }

    ast_fmt_expr(writer, binary_op->right, 0);

    if (binary_op->op == BinaryOpIndex) {
        writer_append_cstr(writer, "]");
    } else {
        writer_append_cstr(writer, ")");
    }
}

void ast_fmt_unary_op(Writer *writer, const AstUnaryOp *unary_op) {
    writer_append_cstr(writer, unary_op_str[unary_op->op]);
    writer_append_cstr(writer, "(");
    ast_fmt_expr(writer, unary_op->expr, 0);
    writer_append_cstr(writer, ")");
}

void ast_fmt_compound_ident(Writer *writer, const Strings *idents) {
    char *sep = "";
    foreach(ident, idents) {
        writer_append_cstr(writer, sep);
        writer_append_string(writer, ident);
        sep = ".";
    }
}

void ast_fmt_call(Writer *writer, const AstCall *call) {
    writer_append_string(writer, &call->name);

    char *sep = "";
    writer_append_cstr(writer, "(");
    foreach(arg, &call->args) {
        writer_append_cstr(writer, sep);
        ast_fmt_expr(writer, arg, 0);
        sep = ", ";
    }
    writer_append_cstr(writer, ")");
}
