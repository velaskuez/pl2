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
#include "util.h"

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
static void gen_location_ident(Generator *self, const AstIdent *ident);
static void gen_location_compound_ident(Generator *self, const AstIdents *idents);
static void gen_location_index(Generator *self, const AstIndex *index);
static void gen_assign(Generator *self, const AstAssign *assign);
static void gen_let(Generator *self, const AstLet *let);
static void gen_return(Generator *self, const AstExpr *return_);
static void gen_if(Generator *self, const AstIf *if_);
static void gen_while(Generator *self, const AstWhile *while_);

static void gen_expr(Generator *self, const AstExpr *expr);
static void gen_binary_op(Generator *self, const AstBinaryOp *binary_op);
static void gen_comparison_op(Generator *self, const char *op_ext, const char *jmp_op_ext);
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

    self->write_fn("%.*s:\n", STRING_FMT_ARGS(&function->name));

    self->variables = init_scoped_variables(self->variables);

    foreach(param, &function->params) {
        i32 local = next_local(self, &param->node);

        // TODO: check.c will guarantee there are no invalid redefinitions
        // but we should still assert that here
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
        gen_let(self, &statement->as.let);
        break;
    case StatementExpr:
        gen_expr(self, &statement->as.expr);
        break;
    case StatementReturn:
        gen_return(self, statement->as.return_);
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

void gen_location_ident(Generator *self, const AstIdent *ident) {
    i32 local = find_variable(self->variables, &ident->name);
    assert(local != -1);

    self->write_fn("load%s %d", op_ext(self, &ident->node), local);
}

void gen_location_compound_ident(Generator *self, const AstIdents *idents) {
}

void gen_location_index(Generator *self, const AstIndex *index) {
}

void gen_assign(Generator *self, const AstAssign *assign) {
    gen_location(self, &assign->location);
    gen_expr(self, &assign->expr);
}

void gen_let(Generator *self, const AstLet *let) {
    i32 local = next_local(self, &let->expr->node);

    // TODO: check.c will guarantee there are no invalid redefinitions
    // but we should still assert that here
    append(self->variables->head, make_variable(let->name, local));

    if (let->expr != nullptr) {
        gen_expr(self, let->expr);
        self->write_fn("store%s %d", op_ext(self, &let->expr->node), local);
    }
}

void gen_return(Generator *self, const AstExpr *expr) {
    if (expr != nullptr) {
        assert(self->ret_ext != nullptr && self->ret_ext[0] != '\0');

        const char *ext = self->ret_ext;
        gen_expr(self, expr);
        self->write_fn("ret%s", ext);
        return;
    }

    self->write_fn("ret");
}

// TODO
void gen_if(Generator *self, const AstIf *if_) {}
void gen_while(Generator *self, const AstWhile *while_) {}

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

void gen_comparison_op(Generator *self, const char *ext, const char *jmp_ext) {
    int l1 = self->label++;
    int l2 = self->label++;

    self->write_fn("cmp%s", ext);
    self->write_fn("jmp%s l%d", jmp_ext, l1);
    self->write_fn("push.w 0");
    self->write_fn("jmp l%d", l2);
    self->write_fn("l%d:", l1);
    self->write_fn("push.w 1");
    self->write_fn("l%d:", l2);
}

void gen_binary_op(Generator *self, const AstBinaryOp *binary_op) {
    // TODO: The same code for a compound identifier location can be re-used for a
    // compound identifier expression, the only difference being the former will do
    // an aload and the latter will do an astore. Compound identifiers could have been
    // parsed as a binary operation, but they weren't and now code reuse is easy. It would
    // be worth reusing the index location type too, but will need tweaking to
    // parse a list of expressions (all of which should resolve/coerce to i64)
    if (binary_op->op == BinaryOpIndex) {
        gen_expr(self, binary_op->left);

        // This should have been checked earlier - the rhs must be I64
        gen_expr(self, binary_op->right);
        self->write_fn("mul.d %d", binary_op->left->node.type.layout.size);
        self->write_fn("aload%s", op_ext(self, &binary_op->left->node));
        return;
    }

    gen_expr(self, binary_op->left);
    gen_expr(self, binary_op->right);

    const char *ext = op_ext(self, &binary_op->left->node);

    switch (binary_op->op) {
    case BinaryOpEq:
        gen_comparison_op(self, ext, ".eq");
		break;
    case BinaryOpNeq:
        gen_comparison_op(self, ext, ".neq");
		break;
    case BinaryOpLt:
        gen_comparison_op(self, ext, ".lt");
		break;
    case BinaryOpLe:
        gen_comparison_op(self, ext, ".le");
		break;
    case BinaryOpGt:
        gen_comparison_op(self, ext, ".gt");
		break;
    case BinaryOpGe:
        gen_comparison_op(self, ext, ".ge");
		break;
    case BinaryOpAdd:
        self->write_fn("add%s", ext);
		break;
    case BinaryOpSub:
        self->write_fn("sub%s", ext);
		break;
    case BinaryOpMul:
        self->write_fn("mul%s", ext);
		break;
    case BinaryOpDiv:
        self->write_fn("div%s", ext);
		break;
    case BinaryOpAnd:
        self->write_fn("add.w");
        self->write_fn("push.w 2");
        gen_comparison_op(self, ".w", ".eq");
		break;
    case BinaryOpOr:
        self->write_fn("add.w");
        self->write_fn("push.w 1");
        gen_comparison_op(self, ".w", ".ge");
		break;
    case BinaryOpBitAnd:
        panic("unimplemented");
		break;
    case BinaryOpBitOr:
        panic("unimplemented");
		break;
    case BinaryOpIndex:
        panic("unreachable");
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

    return local;
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
