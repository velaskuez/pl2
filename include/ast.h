#pragma once

#include "str.h"
#include "token.h"
#include "int.h"
#include "type2.h"

typedef struct AstStatement AstStatement;

typedef struct {
    Position position;

    Type type;
    bool coercible; // Number literals may be coerced to any other number type
} AstNode;

typedef struct {
    size_t len, cap;
    AstStatement *items;
} AstStatements;

typedef struct {
    AstStatements statements;
} AstBlock;

typedef struct {
    String name;
    int pointer;
} AstType;

typedef struct {
    AstNode node;

    String name;
    AstType type;
} AstParam;

typedef struct {
    size_t len, cap;
    AstParam *items;
} AstParams;

typedef struct {
    AstNode node;

    String name;
    AstParams fields;
} AstStruct;

typedef enum {
    BinaryOpEq = 1,
    BinaryOpNeq,
    BinaryOpLt,
    BinaryOpLe,
    BinaryOpGt,
    BinaryOpGe,
    BinaryOpAdd,
    BinaryOpSub,
    BinaryOpMul,
    BinaryOpDiv,
    BinaryOpAnd,
    BinaryOpOr,
    BinaryOpBitAnd,
    BinaryOpBitOr,
    BinaryOpIndex,
} BinaryOp;

char *binary_op_str[BinaryOpIndex+1];

typedef enum {
    UnaryOpSizeOf,
    UnaryOpNew,
} UnaryOp;

char *unary_op_str[UnaryOpNew+1];

typedef struct AstExpr AstExpr;
typedef enum {
    ExprBinaryOp,
    ExprUnaryOp,
    ExprValue,
    ExprIdent,
    ExprCompoundIdent,
    ExprCall,
} ExprKind;

typedef struct {
    AstNode node;

    AstExpr *left;
    BinaryOp op;
    AstExpr *right;
} AstBinaryOp;

typedef struct {
    AstNode node;

    UnaryOp op;
    AstExpr *expr;
} AstUnaryOp;

typedef enum {
    ValueString,
    ValueChar,
    ValueNumber,
} ValueKind;

typedef struct {
    AstNode node;

    ValueKind kind;
    union {
        String string;
        int char_;
        i64 number;
    } as;
} AstValue;

typedef struct {
    size_t len, cap;
    AstExpr *items;
} AstExprs;

typedef struct {
    AstNode node;

    String name;
    AstExprs args;
} AstCall;

typedef struct {
    AstNode node;

    String name;
} AstIdent;

typedef struct {
    size_t len, cap;
    AstIdent *items;
} AstIdents;

typedef struct {
    AstNode node;

    AstIdents idents;
} AstCompoundIdent;

struct AstExpr {
    ExprKind kind;
    union {
        AstBinaryOp binary_op;
        AstUnaryOp unary_op;
        AstValue value;
        AstIdent ident;
        AstCompoundIdent compound_ident;
        AstCall call;
    } as;
};

typedef struct {
    AstNode node;

    String name;
    AstType *type;
    AstExpr *expr;
} AstLet;

typedef enum {
    LocationIdent = 1,
    LocationCompoundIdent,
    LocationIndex
} LocationKind;

typedef struct {
    AstNode node;

    AstIdent ident;
    AstExpr expr;
} AstIndex;

typedef struct {
    LocationKind kind;
    union {
        AstIdent ident;
        AstCompoundIdent compound_ident;
        AstIndex index;
    } as;
} AstLocation;

typedef struct {
    AstLocation location;
    AstExpr expr;
} AstAssign;

typedef struct AstIf AstIf;
struct AstIf {
    AstExpr condition;
    AstBlock block;
    AstIf *else_;
};

typedef struct {
    AstExpr condition;
    AstBlock block;
} AstWhile;

typedef enum {
    StatementAssign,
    StatementLet,
    StatementExpr,
    StatementReturn,
    StatementIf,
    StatementWhile,
} StatementKind;

struct AstStatement {
    StatementKind kind;
    union {
        AstAssign assign;
        AstLet let;
        AstExpr expr;
        AstExpr *return_;
        AstIf if_;
        AstWhile while_;
    } as;
};

typedef struct {
    AstNode node;

    String name;
    AstParams params;
    AstType *return_type;
    AstBlock block;
} AstFunction;

typedef struct {
    size_t len, cap;
    AstStruct *items;
} AstStructs;

typedef struct {
    size_t len, cap;
    AstFunction *items;
} AstFunctions;

typedef struct {
    AstStructs structs;
    AstFunctions functions;
} AstFile;

AstNode *ast_location_node(AstLocation *location);
AstNode *ast_expr_node(AstExpr *expr);
