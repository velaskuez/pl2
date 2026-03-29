#include <assert.h>
#include <setjmp.h>

#include "str.h"
#include "type2.h"
#include "ast.h"
#include "int.h"
#include "util.h"
#include "array.h"
#include "report.h"
#include "symbol2.h"

typedef struct {
    SymbolChain *symbols;
    Report *report;
    Type current_function_return_type;
    bool current_function_returns;
} Checker;

void check_init(Checker *self);
void check_file(Checker *self, const AstFile *file);

static void check_struct(Checker *self, const AstStruct *struct_);
static void check_function(Checker *self, AstFunction *function);
static void check_block(Checker *self, const AstBlock *block);
static void check_statements(Checker *self, const AstStatements *statements);
static void check_statement(Checker *self, AstStatement *statement);
static Type check_ident(Checker *self, AstIdent *ident);
static Type check_compound_ident(Checker *self, AstIdents *idents);
static void check_index(Checker *self, AstIndex *index);
static void check_location(Checker *self, AstLocation *location);
static void check_assign(Checker *self, AstAssign *assign);
static void check_let(Checker *self, AstLet *let);
static Type check_binary_op(Checker *self, AstBinaryOp *binary_op);
static Type check_unary_op(Checker *self, AstUnaryOp *unary_op);
static Type check_value(Checker *self, AstValue *value);
static Type check_call(Checker *self, AstCall *call);
static void check_expr(Checker *self, AstExpr *expr);
static void check_return(Checker *self, AstExpr *expr);
static void check_if(Checker *self, AstIf *if_);
static void check_while(Checker *self, AstWhile *while_);

static void init_scoped_symbols(Checker *self);
static void free_scoped_symbols(Checker *self);
static void report_type_mismatch_error1(Report *self, const Type *want, const Type *have);

// Each AST node that is processed in a loop will use longjmp(buf, -1)
// to avoid having to handle the error at each layer of the AST
jmp_buf struct_jmp_buf, function_jmp_buf, statement_jmp_buf;

void check_init(Checker *self) {
    append(&self->symbols->head, symbol_make_type(string_from_cstr("void"), void_type));
    append(&self->symbols->head, symbol_make_type(string_from_cstr("i8"), i8_type));
    append(&self->symbols->head, symbol_make_type(string_from_cstr("i32"), i32_type));
    append(&self->symbols->head, symbol_make_type(string_from_cstr("i64"), i64_type));
}

void check_file(Checker *self, const AstFile *file) {
    foreach(struct_, &file->structs) {
        int r = setjmp(struct_jmp_buf);
        if (r == -1) continue;

        check_struct(self, struct_);
    }

    foreach(function, &file->functions) {
        int r = setjmp(function_jmp_buf);
        if (r == -1) continue;

        check_function(self, function);
    }
}

void check_struct(Checker *self, const AstStruct *struct_) {
    Strings field_names = {0};
    Types field_types = {0};

    foreach(field, &struct_->fields) {
        Symbol *symbol = symbol_find(self->symbols, &field->name);
        if (symbol == nullptr) {
            TODO("symbol == nullptr");
        }
        if (symbol->kind != TypeSymbol) {
            TODO("symbol->kind != TypeSymbol");
        }

        append(&field_names, symbol->name);
        append(&field_types, symbol->type);
    }

    // TODO: seems redundant to store struct name in Type
    Type type = type_make_struct(struct_->name, &field_types, &field_names);
    append(&self->symbols->head, symbol_make_type(struct_->name, type));
}

void check_function(Checker *self, AstFunction *function) {
    Type return_type = void_type;
    if (function->return_type != nullptr) {
        Symbol *symbol = symbol_find_with_kind(self->symbols, &function->return_type->name, TypeSymbol);
        if (symbol == nullptr) {
            report_error(self->report, "unknown type %.*s", STRING_FMT_ARGS(&function->return_type->name));
            longjmp(function_jmp_buf, -1);
        }

        return_type = symbol->type;
    }

    // TODO: this will be a very simple check for now, but will want to make it
    // a bit more sophisticated at some point
    self->current_function_returns = false;
    self->current_function_return_type = return_type;

    Types argument_types = {0};
    foreach(param, &function->params) {
        Symbol *symbol = symbol_find_with_kind(self->symbols, &param->type.name, TypeSymbol);
        if (symbol == nullptr) {
            report_error(self->report, "unknown type %.*s", STRING_FMT_ARGS(&param->type.name));
            longjmp(function_jmp_buf, -1);
        }

        Type type = param->type.pointer ? type_make_pointer(&symbol->type) : symbol->type;

        param->node.type = type;

        append(&argument_types, type);
    }

    append(&self->symbols->head, symbol_make_function(function->name, return_type, argument_types));

    init_scoped_symbols(self);

    for(size_t i = 0; i < function->params.len; i++) {
        AstParam param = function->params.items[i];
        Type argument_type = argument_types.items[i];

        Symbol *symbol = symbol_find(self->symbols, &param.name);
        if (symbol != nullptr) {
            report_error(self->report, "redefinition of %.*s", STRING_FMT_ARGS(&param.name));
            longjmp(function_jmp_buf, -1);
        }

        append(&self->symbols->head, symbol_make_variable(param.name, argument_type));
    }

    check_block(self, &function->block);

    free_scoped_symbols(self);

    if (!type_equal(&return_type, &void_type) && !self->current_function_returns) {
        report_error(self->report, "%.*s return type is not void - must return a value", STRING_FMT_ARGS(&function->name));
        return;
    }
}

void check_block(Checker *self, const AstBlock *block) {
    init_scoped_symbols(self);
    check_statements(self, &block->statements);
    free_scoped_symbols(self);
}

void check_statements(Checker *self, const AstStatements *statements) {
    foreach(statement, statements) {
        int r = setjmp(statement_jmp_buf);
        if (r == -1) continue;
        check_statement(self, statement);
    }
}

void check_statement(Checker *self, AstStatement *statement) {
    switch (statement->kind) {
    case StatementAssign:
        check_assign(self, &statement->as.assign);
        break;
    case StatementLet:
        check_let(self, &statement->as.let);
        break;
    case StatementExpr:
        check_expr(self, &statement->as.expr);
        break;
    case StatementReturn:
        check_return(self, statement->as.return_);
        break;
    case StatementIf:
        check_if(self, &statement->as.if_);
        break;
    case StatementWhile:
        check_while(self, &statement->as.while_);
        break;
    }
}

Type check_ident(Checker *self, AstIdent *ident) {
    Symbol *symbol = symbol_find_with_kind(self->symbols, &ident->name, VariableSymbol);
    if (symbol == nullptr) {
        report_error(self->report, "unknown identifier %.*s", STRING_FMT_ARGS(&ident->name));
        longjmp(statement_jmp_buf, -1);
    }

    ident->node.type = symbol->type;
    ident->node.coercible = false;

    return symbol->type;
}

Type check_compound_ident(Checker *self, AstIdents *idents) {
    assert(idents->len > 1);

    // TODO: annotate each ident with a type

    AstIdent *base = &idents->items[0];
    check_ident(self, base);

    Type base_type = base->node.type;
    if (base_type.kind != StructType) {
        report_error(self->report, "cannot access fields of %.*s - not a struct", STRING_FMT_ARGS(&base->name));
        longjmp(statement_jmp_buf, -1);
    }

    return base_type;
}

void check_index(Checker *self, AstIndex *index) {
    // Identifier type will be either pointer/array
    // Expression/location type will be the dereferenced type

    check_ident(self, &index->ident);

    Type *type = type_dereference(&index->ident.node.type);
    if (type == nullptr) {
        report_error(self->report, "%.*s cannot be dereferenced", STRING_FMT_ARGS(&index->ident.name));
        longjmp(statement_jmp_buf, -1);
    }

    index->node.type = *type;
    index->node.coercible = false;

    check_expr(self, &index->expr);
    if (!type_equal(&index->expr.node.type, &i64_type)) {
        report_error(self->report, "%.*s cannot be indexed by non-i64 type", STRING_FMT_ARGS(&index->ident.name));
        longjmp(statement_jmp_buf, -1);
    }
}

void check_location(Checker *self, AstLocation *location) {
    switch (location->kind) {
    case LocationIdent:
        check_ident(self, &location->as.ident);
        break;
    case LocationCompoundIdent:
        check_compound_ident(self, &location->as.compound_ident);
        break;
    case LocationIndex:
        check_index(self, &location->as.index);
        break;
    }
}

void check_assign(Checker *self, AstAssign *assign) {
    check_location(self, &assign->location);

    check_expr(self, &assign->expr);

    AstNode *location_node = ast_location_node(&assign->location);
    assert(location_node != nullptr);

    if (!(assign->expr.node.coercible && type_coerce(&assign->expr.node.type, &location_node->type)) && !type_equal(&location_node->type, &assign->expr.node.type)) {
        report_type_mismatch_error1(self->report, &location_node->type, &assign->expr.node.type);
        longjmp(statement_jmp_buf, -1);
    }

    assign->expr.node.type = location_node->type;
    assign->expr.node.coercible = false;
}

void check_let(Checker *self, AstLet *let) {
    Symbol *symbol = symbol_find(self->symbols, &let->name);
    if (symbol != nullptr) {
        report_error(self->report, "cannot redefine %.*s", STRING_FMT_ARGS(&let->name));
        longjmp(statement_jmp_buf, -1);
    }

    check_expr(self, let->expr);

    if (let->type != nullptr) {
        Symbol *symbol = symbol_find_with_kind(self->symbols, &let->type->name, TypeSymbol);
        if (symbol != nullptr) {
            report_error(self->report, "unknown type %.*s", STRING_FMT_ARGS(&let->type->name));
            longjmp(statement_jmp_buf, -1);
        }

        Type type = let->type->pointer ? type_make_pointer(&symbol->type) : symbol->type;

        if (!(let->expr->node.coercible && type_coerce(&let->expr->node.type, &type)) && !type_equal(&let->expr->node.type, &type)) {
            report_type_mismatch_error1(self->report, &type, &let->expr->node.type);
            longjmp(statement_jmp_buf, -1);
        }

        let->expr->node.type = type;
        let->expr->node.coercible = false;
    }

    append(&self->symbols->head, symbol_make_variable(let->name, let->expr->node.type));
}

Type check_binary_op(Checker *self, AstBinaryOp *binary_op) {
    check_expr(self, binary_op->left);
    check_expr(self, binary_op->right);

    // Index - LHS must be indexable, RHS must be i64
    if (binary_op->op == BinaryOpIndex) {
        if (!(binary_op->right->node.coercible && type_coerce(&binary_op->right->node.type, &i64_type)) && !type_equal(&binary_op->right->node.type, &i64_type)) {
            report_error(self->report, "<type> cannot be indexed by non-i64 type");
            longjmp(statement_jmp_buf, -1);
        }

        binary_op->right->node.coercible = false;
        binary_op->right->node.type = i64_type;

        Type *type = type_dereference(&binary_op->left->node.type);
        if (type == nullptr) {
            report_error(self->report, "<type> cannot be indexed");
            longjmp(statement_jmp_buf, -1);
        }

        return *type;
    }

    // For all other operations, operands must be comparable
    if (!type_equal(&binary_op->left->node.type, &binary_op->right->node.type)) {
        bool left_coercible = binary_op->left->node.coercible && type_coerce(&binary_op->left->node.type, &binary_op->right->node.type);
        bool right_coercible = binary_op->right->node.coercible && type_coerce(&binary_op->right->node.type, &binary_op->left->node.type);

        // If they can be coerced in both directions, then they should be equal?
        assert(!(left_coercible && right_coercible));

        if (!left_coercible && !right_coercible) {
            report_type_mismatch_error1(self->report, &binary_op->left->node.type, &binary_op->right->node.type);
        }

        if (left_coercible) {
            binary_op->left->node.type = binary_op->right->node.type;
        } else {
            binary_op->right->node.type = binary_op->left->node.type;
        }
    }

    Type type = {0};
    switch (binary_op->op) {
    case BinaryOpAdd:
    case BinaryOpSub:
    case BinaryOpMul:
    case BinaryOpDiv:
        type = binary_op->left->node.type;
        break;
    case BinaryOpAnd:
    case BinaryOpOr:
    case BinaryOpEq:
    case BinaryOpNeq:
    case BinaryOpLt:
    case BinaryOpLe:
    case BinaryOpGt:
    case BinaryOpGe:
        type = i32_type;
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

    return type;
}

Type check_unary_op(Checker *self, AstUnaryOp *unary_op) {
    // TODO: sizeof can accept expressions too, new can accept type expressions once that's implemented
    assert(unary_op->expr->kind == ExprIdent);
    Symbol *symbol = symbol_find_with_kind(self->symbols, &unary_op->expr->as.ident.name, TypeSymbol);
    if (symbol == nullptr) {
        report_error(self->report, "unknown type %.*s", STRING_FMT_ARGS(&unary_op->expr->as.ident.name));
        longjmp(statement_jmp_buf, -1);
    }

    switch (unary_op->op) {
    case UnaryOpSizeOf:
        return i64_type;
    case UnaryOpNew:
        return type_make_pointer(&symbol->type);
    }
}

Type check_value(Checker *self, AstValue *value) {
    switch (value->kind) {
    case ValueString:
        return type_make_pointer(&i8_type);
    case ValueChar:
        return i32_type;
    case ValueNumber:
        // TODO: It would be nice to store the sign on the value

        // Use the smallest possible type so that it can be coerced
        // to larger types if necessary.
        i64 number = value->as.number;
        if (number >= -128 && number <= 127) {
            return i8_type;
        } else if (number >= -2'147'438'648 && number <= 2'147'438'647) {
            return i32_type;
        } else {
            return i64_type;
        }
    }
}

Type check_call(Checker *self, AstCall *call) {
    Symbol *symbol = symbol_find_with_kind(self->symbols, &call->name, FunctionSymbol);
    if (symbol == nullptr) {
        report_error(self->report, "unknown function %.*s", STRING_FMT_ARGS(&call->name));
        longjmp(statement_jmp_buf, -1);
    }


    Types argument_types = symbol->as.function.argument_types;
    if (call->args.len != argument_types.len) {
        report_error(self->report, "argument count mismatch, want %ld, have %ld", argument_types.len, call->args.len);
        longjmp(statement_jmp_buf, -1);
    }

    size_t i = 0;
    foreach(expr, &call->args) {
        check_expr(self, expr);

        Type type = argument_types.items[i];

        if (!(expr->node.coercible && type_coerce(&expr->node.type, &type)) && !type_equal(&expr->node.type, &type)) {
            report_type_mismatch_error1(self->report, &type, &expr->node.type);
            longjmp(statement_jmp_buf, -1);
        }

        expr->node.type = type;
        expr->node.coercible = false;

        i++;
    }

    return symbol->type;
}

void check_expr(Checker *self, AstExpr *expr) {
    Type type = {0};
    bool coercible = false;

    switch (expr->kind) {
    case ExprBinaryOp:
        type = check_binary_op(self, &expr->as.binary_op);
        coercible = expr->as.binary_op.left->node.coercible && expr->as.binary_op.right->node.coercible;
        break;
    case ExprUnaryOp:
        type = check_unary_op(self, &expr->as.unary_op);
        break;
    case ExprValue:
        type = check_value(self, &expr->as.value);
        coercible = type.kind == PrimitiveType;
        break;
    case ExprIdent:
        type = check_ident(self, &expr->as.ident);
        break;
    case ExprCompoundIdent:
        type = check_compound_ident(self, &expr->as.compound_ident);
        break;
    case ExprCall:
        type = check_call(self, &expr->as.call);
        break;
    }

    expr->node.type = type;
    expr->node.coercible = coercible;
}

void check_return(Checker *self, AstExpr *expr) {
    self->current_function_returns = true;

    if (expr == nullptr) {
        if (!type_equal(&self->current_function_return_type, &void_type)) {
            report_type_mismatch_error1(self->report, &self->current_function_return_type, &void_type);
            longjmp(statement_jmp_buf, -1);
        }

        return;
    }

    check_expr(self, expr);

    if (!type_equal(&self->current_function_return_type, &expr->node.type)) {
        report_type_mismatch_error1(self->report, &self->current_function_return_type, &expr->node.type);
        longjmp(statement_jmp_buf, -1);
    }
}

void check_if(Checker *self, AstIf *if_) {
    return;
}

void check_while(Checker *self, AstWhile *while_) {
    return;
}

void report_type_mismatch_error1(Report *self, const Type *want, const Type *have) {
    report_error(self, "types don't match\n"
                 " ~ want %s\n"
                 " ~ have %s",
                 type_fmt(want),
                 type_fmt(have));
    return;
}

static void init_scoped_symbols(Checker *self) {
    SymbolChain symbols = {0};
    SymbolChain *next = self->symbols;
    self->symbols = box(symbols);
    self->symbols->next = next;
}

static void free_scoped_symbols(Checker *self) {
    SymbolChain *symbols = self->symbols->next;
    free(self->symbols);
    self->symbols = symbols;
}
