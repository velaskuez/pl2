#include <assert.h>
#include <setjmp.h>

#include "check.h"
#include "str.h"
#include "type2.h"
#include "ast.h"
#include "int.h"
#include "util.h"
#include "array.h"
#include "report.h"
#include "symbol2.h"

static void check_struct(Checker *self, const AstStruct *struct_);
static void check_function(Checker *self, AstFunction *function);
static void check_block(Checker *self, const AstBlock *block);
static void check_statements(Checker *self, const AstStatements *statements);
static void check_statement(Checker *self, AstStatement *statement);
static void check_ident(Checker *self, AstIdent *ident);
static void check_compound_ident(Checker *self, AstCompoundIdent *compound_ident);
static void check_index(Checker *self, AstIndex *index);
static void check_location(Checker *self, AstLocation *location);
static void check_assign(Checker *self, AstAssign *assign);
static void check_let(Checker *self, AstLet *let);
static void check_binary_op(Checker *self, AstBinaryOp *binary_op);
static void check_unary_op(Checker *self, AstUnaryOp *unary_op);
static void check_value(Checker *self, AstValue *value);
static void check_call(Checker *self, AstCall *call);
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

void check_init(Checker *self, Report *report) {
    self->report = report;

    init_scoped_symbols(self);

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
        Symbol *symbol = symbol_find_with_kind(self->symbols, &field->type.name, TypeSymbol);
        if (symbol == nullptr) {
            report_error(self->report, "unknown type %.*s", STRING_FMT_ARGS(&field->type.name));
            longjmp(struct_jmp_buf, -1);
        }

        append(&field_names, field->name);

        Type type = field->type.pointer ? type_make_pointer(&symbol->type) : symbol->type;

        append(&field_types, type);
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

    function->node.type = return_type;

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

    // TODO: Need to check branches too
    if (!self->current_function_returns) {
        report_error(self->report, "%.*s does not return", STRING_FMT_ARGS(&function->name));
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

void check_ident(Checker *self, AstIdent *ident) {
    Symbol *symbol = symbol_find_with_kind(self->symbols, &ident->name, VariableSymbol);
    if (symbol == nullptr) {
        report_error(self->report, "unknown identifier %.*s", STRING_FMT_ARGS(&ident->name));
        longjmp(statement_jmp_buf, -1);
    }

    ident->node.type = symbol->type;
    ident->node.coercible = false;
}

void check_compound_ident(Checker *self, AstCompoundIdent *compound_ident) {
    assert(compound_ident->idents.len > 1);

    AstIdent *base = &compound_ident->idents.items[0];
    check_ident(self, base);

    Type base_type = base->node.type;

    for (AstIdent *ident = compound_ident->idents.items+1;
                   ident < compound_ident->idents.items + compound_ident->idents.len;
                   ident++) {

        if (base_type.kind == PointerType) {
            Type *dereferenced_type = type_dereference(&base_type);
            assert(dereferenced_type != nullptr);
            base_type = *dereferenced_type;
        }

        if (base_type.kind != StructType) {
            report_error(self->report, "cannot access fields of %.*s - not a struct", STRING_FMT_ARGS(&base->name));
            longjmp(statement_jmp_buf, -1);
        }

        TypeStructField* field = struct_find_field(&base_type.as.struct_, &ident->name);
        if (field == nullptr) {
            report_error(self->report, "%.*s is not a field of %.*s", STRING_FMT_ARGS(&ident->name), STRING_FMT_ARGS(&base->name));
            longjmp(statement_jmp_buf, -1);
        }

        ident->node.type = *field->type;
        base_type = *field->type;
        base = ident;
    }

    compound_ident->node.type = last(&compound_ident->idents).node.type;
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
    AstNode *expr_node = ast_expr_node(&index->expr);
    if (!type_equal(&expr_node->type, &i64_type)) {
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
    AstNode *expr_node = ast_expr_node(&assign->expr);

    AstNode *location_node = ast_location_node(&assign->location);
    assert(location_node != nullptr);

    if (!(expr_node->coercible && type_coerce(&expr_node->type, &location_node->type)) && !type_equal(&location_node->type, &expr_node->type)) {
        report_type_mismatch_error1(self->report, &location_node->type, &expr_node->type);
        longjmp(statement_jmp_buf, -1);
    }

    expr_node->type = location_node->type;
    expr_node->coercible = false;
}

void check_let(Checker *self, AstLet *let) {
    Symbol *symbol = symbol_find(self->symbols, &let->name);
    if (symbol != nullptr) {
        report_error(self->report, "cannot redefine %.*s", STRING_FMT_ARGS(&let->name));
        longjmp(statement_jmp_buf, -1);
    }

    if (let->type == nullptr && let->expr == nullptr) {
        report_error(self->report, "cannot declare %.*s without type or expression", STRING_FMT_ARGS(&let->name));
        longjmp(statement_jmp_buf, -1);
    }

    AstNode *expr_node = nullptr;
    if (let->expr != nullptr) {
        check_expr(self, let->expr);
        expr_node = ast_expr_node(let->expr);
    }

    if (let->type == nullptr) {
        let->node.type = expr_node->type;
        append(&self->symbols->head, symbol_make_variable(let->name, let->node.type));
        return;
    }

    // At this point, let->type is not null, expr might be null
    symbol = symbol_find_with_kind(self->symbols, &let->type->name, TypeSymbol);
    if (symbol == nullptr) {
        report_error(self->report, "unknown type %.*s", STRING_FMT_ARGS(&let->type->name));
        longjmp(statement_jmp_buf, -1);
    }

    if (symbol->type.kind == PrimitiveType && symbol->type.as.primitive.kind == PrimitiveVoid) {
        report_error(self->report, "cannot declare %.*s with type void", STRING_FMT_ARGS(&let->type->name));
        longjmp(statement_jmp_buf, -1);
    }

    Type let_type = let->type->pointer ? type_make_pointer(&symbol->type) : symbol->type;

    if (expr_node != nullptr) {
        if (!(expr_node->coercible && type_coerce(&expr_node->type, &let_type)) && !type_equal(&expr_node->type, &let_type)) {
            report_type_mismatch_error1(self->report, &let_type, &expr_node->type);
            longjmp(statement_jmp_buf, -1);
        }

        expr_node->type = let_type;
        expr_node->coercible = false;
    }

    let->node.type = let_type;
    append(&self->symbols->head, symbol_make_variable(let->name, let_type));
}

void check_binary_op(Checker *self, AstBinaryOp *binary_op) {
    check_expr(self, binary_op->left);
    AstNode *lhs_node = ast_expr_node(binary_op->left);

    check_expr(self, binary_op->right);
    AstNode *rhs_node = ast_expr_node(binary_op->right);

    // Index - LHS must be indexable, RHS must be i64
    if (binary_op->op == BinaryOpIndex) {
        if (!(rhs_node->coercible && type_coerce(&rhs_node->type, &i64_type)) && !type_equal(&rhs_node->type, &i64_type)) {
            report_error(self->report, "<type> cannot be indexed by non-i64 type");
            longjmp(statement_jmp_buf, -1);
        }

        rhs_node->type = i64_type;
        rhs_node->coercible = false;

        Type *type = type_dereference(&lhs_node->type);
        if (type == nullptr) {
            report_error(self->report, "<type> cannot be indexed");
            longjmp(statement_jmp_buf, -1);
        }

        binary_op->node.type = *type;
        binary_op->node.coercible = false;

        return;
    }

    // For all other operations, operands must be comparable
    if (!type_equal(&lhs_node->type, &rhs_node->type)) {
        bool left_coercible = lhs_node->coercible && type_coerce(&lhs_node->type, &rhs_node->type);
        bool right_coercible = rhs_node->coercible && type_coerce(&rhs_node->type, &lhs_node->type);

        // If they can be coerced in both directions, then they should be equal?
        assert(!(left_coercible && right_coercible));

        if (!left_coercible && !right_coercible) {
            report_type_mismatch_error1(self->report, &lhs_node->type, &rhs_node->type);
        }

        if (left_coercible) {
            lhs_node->type = rhs_node->type;
        } else {
            rhs_node->type = lhs_node->type;
        }
    }

    Type type = {0};
    switch (binary_op->op) {
    case BinaryOpAdd:
    case BinaryOpSub:
    case BinaryOpMul:
    case BinaryOpDiv:
        type = lhs_node->type;
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

    binary_op->node.type = type;
    binary_op->node.coercible = lhs_node->coercible && rhs_node->coercible;
}

void check_unary_op(Checker *self, AstUnaryOp *unary_op) {
    // TODO: sizeof can accept expressions too, new can accept type expressions once that's implemented
    assert(unary_op->expr->kind == ExprIdent);
    Symbol *symbol = symbol_find_with_kind(self->symbols, &unary_op->expr->as.ident.name, TypeSymbol);
    if (symbol == nullptr) {
        report_error(self->report, "unknown type %.*s", STRING_FMT_ARGS(&unary_op->expr->as.ident.name));
        longjmp(statement_jmp_buf, -1);
    }

    Type type = {0};
    switch (unary_op->op) {
    case UnaryOpSizeOf:
        type = i64_type;
        break;
    case UnaryOpNew:
        type = type_make_pointer(&symbol->type);
        break;
    }

    unary_op->node.type = type;
    unary_op->node.coercible = false;
}

void check_value(Checker *self, AstValue *value) {
    Type type = {0};
    bool coercible = true;

    switch (value->kind) {
    case ValueString:
        type = type_make_pointer(&i8_type);
        coercible = false;
        break;
    case ValueChar:
        type = i32_type;
        break;
    case ValueNumber:
        // TODO: It would be nice to store the sign on the value

        // Use the smallest possible type so that it can be coerced
        // to larger types if necessary.
        i64 number = value->as.number;
        if (number >= -128 && number <= 127) {
            type = i8_type;
        } else if (number >= -2'147'438'648 && number <= 2'147'438'647) {
            type = i32_type;
        } else {
            type = i64_type;
        }

        break;
    }

    value->node.type = type;
    value->node.coercible = coercible;
}

void check_call(Checker *self, AstCall *call) {
    Symbol *symbol = symbol_find_with_kind(self->symbols, &call->name, FunctionSymbol);
    if (symbol == nullptr) {
        report_error(self->report, "unknown function %.*s", STRING_FMT_ARGS(&call->name));
        longjmp(statement_jmp_buf, -1);
    }

    call->node.type = symbol->type;
    call->node.coercible = false;

    Types argument_types = symbol->as.function.argument_types;
    if (call->args.len != argument_types.len) {
        report_error(self->report, "argument count mismatch, want %ld, have %ld", argument_types.len, call->args.len);
        longjmp(statement_jmp_buf, -1);
    }

    size_t i = 0;
    foreach(expr, &call->args) {
        check_expr(self, expr);
        AstNode *expr_node = ast_expr_node(expr);

        Type type = argument_types.items[i];

        if (!(expr_node->coercible && type_coerce(&expr_node->type, &type)) && !type_equal(&expr_node->type, &type)) {
            report_type_mismatch_error1(self->report, &type, &expr_node->type);
            longjmp(statement_jmp_buf, -1);
        }

        expr_node->type = type;
        expr_node->coercible = false;

        i++;
    }
}

void check_expr(Checker *self, AstExpr *expr) {
    switch (expr->kind) {
    case ExprBinaryOp:
        check_binary_op(self, &expr->as.binary_op);
        break;
    case ExprUnaryOp:
        check_unary_op(self, &expr->as.unary_op);
        break;
    case ExprValue:
        check_value(self, &expr->as.value);
        break;
    case ExprIdent:
        check_ident(self, &expr->as.ident);
        break;
    case ExprCompoundIdent:
        check_compound_ident(self, &expr->as.compound_ident);
        break;
    case ExprCall:
        check_call(self, &expr->as.call);
        break;
    }
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
    AstNode *expr_node = ast_expr_node(expr);

    if (!type_equal(&self->current_function_return_type, &expr_node->type)) {
        report_type_mismatch_error1(self->report, &self->current_function_return_type, &expr_node->type);
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
