#include <_string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "token.h"
#include "ast.h"

#define box(data) ({                           \
    auto value = data;                         \
    void *allocation = malloc(sizeof(value));  \
    memcpy(allocation, &value, sizeof(value)); \
    allocation;                                \
})

static Token nth(Parser *self, size_t n);
static Token current(Parser *self);
static int at(Parser *self, TokenKind kind);
static int eat(Parser *self, TokenKind want);
static Token expect(Parser *self, TokenKind want);
static int eof(Parser *self);

static AstStruct parse_struct(Parser *self);

static AstExpr parse_expr(Parser *self, int prec);
static AstType parse_type(Parser *self);
static AstParam parse_param(Parser *self);
static AstParams parse_params(Parser *self);
static AstLet parse_let(Parser *self);
static AstIf parse_if(Parser *self);
static AstWhile parse_while(Parser *self);
static AstExpr parse_return(Parser *self);
static AstStatement parse_statement(Parser *self);
static AstStatements parse_statements(Parser *self);
static AstBlock parse_block(Parser *self);
static AstFunction parse_function(Parser *self);

AstFile parse(Parser *self) {
    AstFile file = {};

    while (!eof(self)) {
        if (at(self, KeywordFn)) {
            AstFunction function = parse_function(self);
            append(file.functions, function);
        } else if (at(self, KeywordStruct)) {
            AstStruct struct_ = parse_struct(self);
            append(file.structs, struct_);
        } else {
            // unexpected token
        }
    }

    return file;
}

Token nth(Parser *self, size_t n) {
    if (self->position + n >= self->tokens.len)
        return (Token){.kind = TokenEof, .value = {0}};

    return self->tokens.items[self->position+n];
}

Token current(Parser *self) {
    return nth(self, 0);
}

int at(Parser *self, TokenKind kind) {
    return current(self).kind == kind;
}

int eat(Parser *self, TokenKind want) {
    if (!at(self, want)) return false;

    self->position++;
    return true;
}

Token expect(Parser *self, TokenKind want) {
    if (at(self, want)) {
        Token token = current(self);
        self->position++;
        return token;
    }

    // TODO
    fprintf(stderr, "unexpected token: %d, want: %d", current(self).kind, want);
    exit(1);
}

int eof(Parser *self) {
    return current(self).kind == TokenEof;
}

AstValue parse_value(Parser *self) {
    AstValue value = {0};

    if (at(self, TokenString)) {
        value.kind = ValueString;
        value.as.string = expect(self, TokenString).value;
    } else if (at(self, TokenChar)) {
        value.kind = ValueChar;
        value.as.char_ = expect(self, TokenChar).value.items[0];
    } else if (at(self, TokenNumber)) {
        value.kind = ValueNumber;
        String number = expect(self, TokenNumber).value;
        value.as.number = strtoll(number.items, nullptr, 10);
    } else {
        // TODO
    }

    return value;
}

AstCall parse_call(Parser *self) {
    AstCall call = {0};

    call.name = expect(self, TokenIdent).value;

    while (!at(self, TokenLParen) && !eof(self)) {
        AstExpr arg = parse_expr(self, 0);
        append(&call.args, arg);
        if (at(self, TokenComma)) break;
    }

    expect(self, TokenRParen);

    return call;
}

Strings parse_compound_ident(Parser *self) {
    Strings idents = {0};

    while (!eof(self)) {
        String ident = expect(self, TokenIdent).value;
        append(&idents, ident);
        if (!eat(self, TokenDot)) break;
    }

    return idents;
}

AstExpr parse_prefix(Parser *self) {
    AstExpr expr = {0};

    if (eat(self, TokenLParen)) {
        expr = parse_expr(self, 0);
        expect(self, TokenRParen);
    } else if (at(self, TokenString) || at(self, TokenChar) || at(self, TokenNumber)) {
        expr.kind = ExprValue;
        expr.as.value = parse_value(self);
    } else if (at(self, TokenIdent) && nth(self, 1).kind == TokenLParen) {
        expr.kind = ExprCall;
        expr.as.call = parse_call(self);
    } else if (at(self, TokenIdent) && nth(self, 1).kind == TokenDot) {
        expr.kind = ExprCompoundIdent;
        expr.as.compound_ident = parse_compound_ident(self);
    } else if (at(self, TokenIdent)) {
        expr.kind = ExprIdent;
        expr.as.ident = expect(self, TokenIdent).value;
    } else {
        // TODO
    }

    return expr;
}

AstExpr parse_expr(Parser *self, int prec) {
    AstExpr expr = {0};

}

AstType parse_type(Parser *self) {
    AstType type = {0};
    type.pointer = eat(self, TokenStar);
    type.name = expect(self, TokenIdent).value;

    return type;
}

AstParam parse_param(Parser *self) {
    AstParam param = {0};
    param.name = expect(self, TokenIdent).value;
    param.type.name = expect(self, TokenIdent).value;

    return param;
}

AstParams parse_params(Parser *self) {
    AstParams params = {0};

    while (!at(self, TokenRParen) && !eof(self)) {
        AstParam param = parse_param(self);
        append(&params, param);
        if (!eat(self, TokenComma)) break;
    }

    return params;
}

AstLet parse_let(Parser *self) {
    AstLet let = {0};

    expect(self, KeywordLet);

    let.name = expect(self, TokenIdent).value;

    if (at(self, TokenIdent) || at(self, TokenStar)) {
        let.type = box(parse_type(self));
    }

    if (!at(self, TokenSemicolon)) {
        let.expr = box(parse_expr(self, 0));
    }

    return let;
}

AstIf parse_if(Parser *self) {
    AstIf if_ = {0};

    expect(self, KeywordIf);

    int expect_r_paren = eat(self, TokenLParen);
    if_.condition = parse_expr(self, 0);
    if (expect_r_paren) expect(self, TokenRParen);

    if (eat(self, KeywordElse)) {
        if_.else_ = box(parse_if(self));
    }

    return if_;
}

AstWhile parse_while(Parser *self) {
    AstWhile while_ = {0};

    expect(self, KeywordWhile);

    int expect_r_paren = eat(self, TokenLParen);
    while_.condition = parse_expr(self, 0);
    if (expect_r_paren) expect(self, TokenRParen);

    while_.block = parse_block(self);

    return while_;
}

AstExpr parse_return(Parser *self) {
    AstExpr expr = {0};
    expect(self, KeywordReturn);
    expr = parse_expr(self, 0);

    return expr;
}

AstStatement parse_statement(Parser *self) {
    AstStatement statement = {0};

    if (at(self, KeywordLet)) {
        statement.kind = StatementLet;
        statement.as.let = parse_let(self);
    } else if (at(self, KeywordIf)) {
        statement.kind = StatementIf;
        statement.as.if_ = parse_if(self);
    } else if (at(self, KeywordWhile)) {
        statement.kind = StatementWhile;
        statement.as.while_ = parse_while(self);
    } else if (at(self, KeywordReturn)) {
        statement.kind = StatementReturn;
        statement.as.return_ = parse_return(self);
    } else if (at(self, TokenIdent)) {
        // could be lvalue, check = and [
        TokenKind next_token = nth(self, 1).kind;
        if (next_token == TokenEqual) {
            statement.kind = StatementAssign;
        } else if (next_token == TokenLBrack) {
            // parse_lvalue -> ident || ident[expr]; return lvalue, eat(Equal);
        } else goto expr;
    } else {
expr:
        statement.kind = StatementExpr;
        statement.as.expr = parse_expr(self, 0);
    }

    return statement;
}

AstStatements parse_statements(Parser *self) {
    AstStatements statements = {0};

    while (!eof(self)) {
        AstStatement statement = parse_statement(self);
        append(&statements, statement);
        expect(self, TokenSemicolon);
    }

    return statements;
}

AstBlock parse_block(Parser *self) {
    AstBlock block = {0};
    expect(self, TokenLCurly);
    block.statements = parse_statements(self);
    expect(self, TokenRCurly);

    return block;
}

AstFunction parse_function(Parser *self) {
    expect(self, KeywordFn);

    AstFunction function = {0};
    function.name = expect(self, TokenIdent).value;

    expect(self, TokenLParen);
    function.params = parse_params(self);
    expect(self, TokenRParen);


    if (at(self, TokenIdent)) {
        function.return_type = parse_type(self);
    }

    function.block = parse_block(self);

    return function;
}
