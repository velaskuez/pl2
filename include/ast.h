#pragma once

#include "str.h"
#include "token.h"

typedef struct AstStatement AstStatement;

typedef struct  {
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
    String name;
    AstType type;
} AstParam;

typedef struct {
    size_t len, cap;
    AstParam *items;
} AstParams;

typedef struct {
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

typedef enum {
    UnaryOpSizeOf,
} UnaryOp;

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
    AstExpr *left;
    BinaryOp op;
    AstExpr *right;
} AstBinaryOp;

typedef struct {
    UnaryOp op;
    AstExpr *expr;
} AstUnaryOp;

typedef enum {
    ValueString,
    ValueChar,
    ValueNumber,
} ValueKind;

typedef struct {
    ValueKind kind;
    union {
        String string;
        int char_;
        long number;
    } as;
} AstValue;

typedef struct {
    size_t len, cap;
    AstExpr *items;
} AstExprs;

typedef struct {
    String name;
    AstExprs args;
} AstCall;

struct AstExpr{
    ExprKind kind;
    union {
        AstBinaryOp binary_op;
        AstUnaryOp unary_op;
        AstValue value;
        String ident;
        Strings compound_ident;
        AstCall call;
    } as;
};

typedef struct {
    String name;
    AstType *type;
    AstExpr *expr;
} AstLet;

typedef struct {
    String name;
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
        AstExpr return_;
        AstIf if_;
        AstWhile while_;
    } as;
};

typedef struct {
    String name;
    AstParams params;
    AstType return_type;
    AstBlock block;
} AstFunction;

typedef struct {
    Tokens tokens;
    size_t position;
} Parser;

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

AstFile parse_file(Parser *self);
