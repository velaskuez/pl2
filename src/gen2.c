#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "array.h"
#include "str.h"
#include "type2.h"
#include "symbol2.h"
#include "ast.h"
#include "report.h"

typedef struct {
    String name;
    i32 local;
} Variable;

Variable make_variable(String name, i32 local) {
    return (Variable) { .name = name, .local = local };
}

typedef struct {
    size_t cap, len;
    Variable *items;
} Variables;

typedef struct VariableChain VariableChain;
struct VariableChain {
    Variables *head;
    VariableChain *next;
};

i32 find_variable(VariableChain *self, const String *name) {
    if (self == nullptr) return -1;

    foreach(variable, self->head) {
        if (string_cmp(&variable->name, name) == 0) {
            return variable->local;
        }
    }

    return find_variable(self->next, name);
}

VariableChain* init_scoped_variables(VariableChain *self) {
    VariableChain *head = calloc(1, sizeof(VariableChain));
    head->next = self;
    return head;
}

VariableChain* free_scoped_variables(VariableChain *self) {
    VariableChain *head = self->next;
    free(self->head);
    return head;
}

static int printf_with_newline(const char* fmt, ...) {
    int n = 0;

    va_list args;
    va_start(args, fmt);
    n += vprintf(fmt, args);
    va_end(args);
    n += printf("\n");

    return n;
}


typedef struct {
    VariableChain *variables;

    int local; // For allocating temporary variables
    int string; // For assigning labels to static strings
    int label; // For allocating labels for control flow

    char *ret_ext;

    int (*write_fn)(const char *, ...) __attribute__((format(printf, 1, 2)));

    Report *report;
} Generator;

void gen_init(Generator *self, Report *report);
void gen_file(Generator *self, const AstFile *file);

static void gen_function(Generator *self, const AstFunction *function);
static void gen_block(Generator *self, const AstBlock *block);
static void gen_statements(Generator *self, const AstStatements *statements);
static void gen_statement(Generator *self, const AstStatement *statement);
static void gen_location(Generator *self, const AstLocation *location);
static void gen_location_ident(Generator *self, const String *ident);
static void gen_location_compound_ident(Generator *self, const Strings *idents);
static void gen_location_index(Generator *self, const AstIndex *index);
static void gen_assign(Generator *self, const AstAssign *assign);
// static void gen_let(Generator *self, const AstLet *let);
// static void gen_statement_expr(Generator *self, const AstExpr *return_);
// static void gen_return(Generator *self, const AstExpr *return_);
// static void gen_if(Generator *self, const AstIf *if_);
// static void gen_while(Generator *self, const AstWhile *while_);

static void gen_expr(Generator *self, const AstExpr *expr);
static void gen_binary_op(Generator *self, const AstBinaryOp *binary_op);
// static void gen_comparison_op(Generator *self, const char *op_ext, const char *jmp_op_ext);
// static void gen_unary_op_new(Generator *self, const AstExpr *expr);
// static void gen_unary_op(Generator *self, const AstUnaryOp *unary_op);
static void gen_value(Generator *self, const AstValue *value, const AstNode *node);
// static void gen_ident(Generator *self, const String *ident);
// static void gen_compound_ident(Generator *self, const Strings *idents);
// static void gen_call(Generator *self, const AstCall *call);

jmp_buf fail_buf;

static i32 next_local(Generator *self, const AstNode *node);
static char *op_ext(Generator *self, const AstNode *node);
static char *ret_ext(Generator *self, const AstNode *node);

void gen_init(Generator *self, Report *report) {
    self->variables = init_scoped_variables(nullptr);
    self->write_fn = printf_with_newline;
    self->report = report;
}

void gen_file(Generator *self, const AstFile *file) {
    self->write_fn(".entry main");

    int r = setjmp(fail_buf);
    if (r == -1) {
        report_error(self->report, "failed to generate stack");
        return;
    }

    foreach(function, &file->functions) {
        gen_function(self, function);
    }
}

void gen_function(Generator *self, const AstFunction *function) {
    int local = self->local;
    self->ret_ext = ret_ext(self, &function->node);

    self->write_fn("%.*s:\n", function->name);

    self->variables = init_scoped_variables(self->variables);

    foreach(param, &function->params) {
        i32 local = next_local(self, &param->node);
        append(self->variables->head, make_variable(param->name, local));
    }

    gen_block(self, &function->block);

    self->variables = free_scoped_variables(self->variables);

    self->local = local;
    self->ret_ext = nullptr;
}

void gen_block(Generator *self, const AstBlock *block) {
    self->variables = init_scoped_variables(self->variables);
    gen_statements(self, &block->statements);
    self->variables = free_scoped_variables(self->variables);
}

void gen_statements(Generator *self, const AstStatements *statements) {
    foreach(statement, statements) {
        gen_statement(self, statement);
    }
}

void gen_statement(Generator *self, const AstStatement *statement) {
    switch (statement->kind) {
    case StatementAssign:
        gen_assign(self, &statement->as.assign);
        break;
    case StatementLet:
        break;
    case StatementExpr:
        break;
    case StatementReturn:
        break;
    case StatementIf:
        break;
    case StatementWhile:
        break;
    }
}

void gen_location(Generator *self, const AstLocation *location) {
    switch (location->kind) {
    case LocationIdent:
        gen_location_ident(self, &location->as.ident);
		break;
    case LocationCompoundIdent:
        gen_location_compound_ident(self, &location->as.compound_ident);
		break;
    case LocationIndex:
        gen_location_index(self, &location->as.index);
		break;
    }
}

void gen_location_ident(Generator *self, const String *ident) {
    i32 local = find_variable(self->variables, ident);
    assert(local != -1);

    self->write_fn("load%s %d", op_ext(self, nullptr)); // TODO: AstNode for identifiers
}

void gen_location_compound_ident(Generator *self, const Strings *idents) {
}

void gen_location_index(Generator *self, const AstIndex *index) {
}

void gen_assign(Generator *self, const AstAssign *assign) {
    gen_location(self, &assign->location);
    gen_expr(self, &assign->expr);
}

void gen_expr(Generator *self, const AstExpr *expr) {
    switch (expr->kind) {
    case ExprBinaryOp:
        gen_binary_op(self, &expr->as.binary_op);
		break;
    case ExprUnaryOp:
		break;
    case ExprValue:
        gen_value(self, &expr->as.value, &expr->node);
		break;
    case ExprIdent:
		break;
    case ExprCompoundIdent:
		break;
    case ExprCall:
		break;
    }
}

void gen_binary_op(Generator *self, const AstBinaryOp *binary_op) {
    gen_expr(self, binary_op->left);
    gen_expr(self, binary_op->right);

    switch (binary_op->op) {
    case BinaryOpEq:
		break;
    case BinaryOpNeq:
		break;
    case BinaryOpLt:
		break;
    case BinaryOpLe:
		break;
    case BinaryOpGt:
		break;
    case BinaryOpGe:
		break;
    case BinaryOpAdd:
        self->write_fn("add%s", op_ext(self, &binary_op->left->node));
		break;
    case BinaryOpSub:
        self->write_fn("sub%s", op_ext(self, &binary_op->left->node));
		break;
    case BinaryOpMul:
        self->write_fn("mul%s", op_ext(self, &binary_op->left->node));
		break;
    case BinaryOpDiv:
        self->write_fn("div%s", op_ext(self, &binary_op->left->node));
		break;
    case BinaryOpAnd:
		break;
    case BinaryOpOr:
		break;
    case BinaryOpBitAnd:
		break;
    case BinaryOpBitOr:
		break;
    case BinaryOpIndex:
		break;
    }
}

void gen_value(Generator *self, const AstValue *value, const AstNode *node) {
    switch (value->kind) {
    case ValueString:
        int label = self->string++;
        self->write_fn(".data s%d .string \"%.*s\"", label, STRING_FMT_ARGS(&value->as.string));
        self->write_fn("dataptr s%d", label);
        break;
    case ValueChar:
        self->write_fn("push%s %d", op_ext(self, node), value->as.char_);
        break;
    case ValueNumber:
        self->write_fn("push%s %ld", op_ext(self, node), value->as.number);
    }
}

i32 next_local(Generator *self, const AstNode *node) {
    int local = self->local;

    switch (node->type.kind) {
    case StructType:
        report_error(self->report, "attempt to generate local for struct");
        longjmp(fail_buf, -1);
        break;
    case PrimitiveType:
        switch (node->type.as.primitive.kind) {
        case PrimitiveVoid:
            report_error(self->report, "attempt to generate local for void");
            longjmp(fail_buf, -1);
            break;
        case PrimitiveI8:
        case PrimitiveI32:
            self->local += 1;
            self->local += 1;
            break;
        case PrimitiveI64:
            self->local += 2;
            break;
        }
    case PointerType:
    case ArrayType:
        self->local += 2;
        break;
    }
}

char *op_ext(Generator *self, const AstNode *node) {
    switch (node->type.kind) {
    case StructType:
        report_error(self->report, "cannot use structs as operands in stack - access individual fields instead");
        longjmp(fail_buf, -1);
        break;
    case PrimitiveType:
        switch (node->type.as.primitive.kind) {
        case PrimitiveVoid:
            report_error(self->report, "cannot use void operands");
            longjmp(fail_buf, -1);
            break;
        case PrimitiveI8:
            return ".b";
        case PrimitiveI32:
            return ".w";
        case PrimitiveI64:
            return ".d";
        }
    case PointerType:
    case ArrayType:
        return ".d";
    }
}

char *ret_ext(Generator *self, const AstNode *node) {
    switch (node->type.kind) {
    case StructType:
        report_error(self->report, "cannot return objects in stack - return a pointer instead");
        longjmp(fail_buf, -1);
        break;
    case PrimitiveType:
        switch (node->type.as.primitive.kind) {
        case PrimitiveVoid:
            return "";
        case PrimitiveI8:
        case PrimitiveI32:
            return ".w";
        case PrimitiveI64:
            return ".d";
        }
    case PointerType:
    case ArrayType:
        return ".d";
    }
}
