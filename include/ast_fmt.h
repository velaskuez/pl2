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
void ast_fmt_statement_return(Writer *writer, const AstExpr *expr, int indent);
void ast_fmt_expr(Writer *writer, const AstExpr *expr, int indent);
