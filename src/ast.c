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
        return &expr->as.binary_op.node;
    case ExprUnaryOp:
        return &expr->as.unary_op.node;
    case ExprValue:
        return &expr->as.value.node;
    case ExprIdent:
        return &expr->as.ident.node;
    case ExprCompoundIdent:
        return &expr->as.compound_ident.node;
    case ExprCall:
        return &expr->as.call.node;
    }

    return nullptr;
}

const String *ast_type_name(const AstTypeExpr *type_expr) {
    switch (type_expr->kind) {
    case IdentTypeExpr:
        return &type_expr->as.ident.name;
    case PointerTypeExpr:
        return ast_type_name(type_expr->as.pointer_to);
    case ArrayTypeExpr:
        return ast_type_name(type_expr->as.array.of);
    }
}
