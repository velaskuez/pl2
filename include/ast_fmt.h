#pragma once

#include "ast.h"
#include "writer.h"

void ast_fmt_file(Writer *writer, const AstFile *file);

void ast_fmt_struct(Writer *writer, const AstStruct *file, int indent);
void ast_fmt_function(Writer *writer, const AstFunction *file, int indent);
void ast_fmt_type(Writer *writer, const AstType *type);
void ast_fmt_param(Writer *writer, const AstParam *param, int indent);

void ast_fmt_block(Writer *writer, const AstBlock *block, int indent);
void ast_fmt_statement(Writer *writer, const AstStatement *statement, int indent);
void ast_fmt_return(Writer *writer, const AstExpr *expr, int indent);
void ast_fmt_index(Writer *writer, const AstIndex *index);
void ast_fmt_location(Writer *writer, const AstLocation *location);
void ast_fmt_assign(Writer *writer, const AstAssign *assign, int indent);
void ast_fmt_let(Writer *writer, const AstLet *let, int indent);
void ast_fmt_if(Writer *writer, const AstIf *if_, int indent);
void ast_fmt_while(Writer *writer, const AstWhile *while_, int indent);

void ast_fmt_expr(Writer *writer, const AstExpr *expr, int indent);
void ast_fmt_value(Writer *writer, const AstValue *value);
void ast_fmt_binary_op(Writer *writer, const AstBinaryOp *binary_op);
void ast_fmt_unary_op(Writer *writer, const AstUnaryOp *unary_op);
void ast_fmt_compound_ident(Writer *writer, const Strings *idents);
void ast_fmt_call(Writer *writer, const AstCall *call);
