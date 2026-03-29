#include "ast.h"

AstNode *ast_location_node(AstLocation *location) {
    switch (location->kind) {
    case LocationIdent:
        return &location->as.ident.node;
    case LocationCompoundIdent:
        return &location->as.compound_ident.node;
    case LocationIndex:
        return &location->as.index.node;
    }
}

AstNode *ast_expr_node(AstExpr *expr) {
    switch (expr->kind) {
    case ExprBinaryOp:
        break;
    case ExprUnaryOp:
        break;
    case ExprValue:
        break;
    case ExprIdent:
        break;
    case ExprCompoundIdent:
        break;
    case ExprCall:
        break;
    }

    return nullptr;
}
