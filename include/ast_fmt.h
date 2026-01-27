#pragma once

#include "ast.h"
#include "writer.h"

// TODO: it would be better if Writer was the first param
void ast_fmt_file(const AstFile *file, Writer *output);
void ast_fmt_struct(const AstStruct *file, Writer *output, int indent);
void ast_fmt_function(const AstFunction *file, Writer *output, int indent);
void ast_fmt_type(const AstType *type, Writer *writer);
void ast_fmt_param(const AstParam *param, Writer *output, int indent);
void ast_fmt_block(const AstBlock *block, Writer *output, int indent);

void ast_fmt_statement(const AstStatement *statement, Writer *output, int indent);
void ast_fmt_statement_return(const AstExpr *expr, Writer *output, int indent);
void ast_fmt_expr(const AstExpr *expr, Writer *output, int indent);
