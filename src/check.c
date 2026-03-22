#include <assert.h>
#include <setjmp.h>

#include "str.h"
#include "type2.h"
#include "ast.h"
#include "int.h"
#include "util.h"
#include "array.h"
#include "report.h"

typedef enum {
    FunctionSymbol,
    VariableSymbol,
    TypeSymbol,
} SymbolKind;

char *symbol_str[] = {
    [FunctionSymbol] = "function",
    [VariableSymbol] = "variable",
    [TypeSymbol] = "type"
};

typedef struct {
    u32 local;
} SymbolVariable;

typedef struct {
    Types argument_types;
} SymbolFunction;

typedef struct {
    SymbolKind kind;
    String name;
    Type type;
    union {
        SymbolFunction function;
        SymbolVariable variable;
    } as;
} Symbol;

typedef struct {
    size_t len, cap;
    Symbol *items;
} Symbols;

typedef struct SymbolChain SymbolChain;
struct SymbolChain {
    Symbols head;
    SymbolChain *next;
};

Symbol *symbol_find(const SymbolChain *self, const String *key) {
    if (self == nullptr) return nullptr;

    foreach(symbol, &self->head) {
        if (string_cmp(&symbol->name, key) == 0) {
            return symbol;
        }
    }

    return symbol_find(self->next, key);
}

Symbol *symbol_find_with_kind(const SymbolChain *self, const String *key, SymbolKind kind) {
    Symbol *symbol =  symbol_find(self->next, key);
    if (symbol == nullptr || symbol->kind != kind) return nullptr;
    return symbol;
}

Symbol symbol_make_variable(String name, Type type) {
    Symbol symbol = {0};
    symbol.name = name;
    symbol.kind = VariableSymbol;
    symbol.type = type;
    symbol.as.variable.local = 0; // Is this needed here?

    return symbol;
}

Symbol symbol_make_function(String name, Type type, Types argument_type) {
    Symbol symbol = {0};
    symbol.name = name;
    symbol.kind = FunctionSymbol;
    symbol.type = type;
    symbol.as.function.argument_types = argument_type;

    return symbol;
}

Symbol symbol_make_type(String name, Type type) {
    Symbol symbol = {0};
    symbol.name = name;
    symbol.kind = TypeSymbol;
    symbol.type = type;

    return symbol;
}

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
static Type check_ident(Checker *self, const String *ident);
static Type check_compount_ident(Checker *self, const Strings *idents);
static Type check_index(Checker *self, AstIndex *index);
static void check_location(Checker *self, AstLocation *location);
static void check_assign(Checker *self, AstAssign *assign);
static void check_let(Checker *self, AstLet *let);
static void check_expr(Checker *self, AstExpr *expr);
static void check_return(Checker *self, AstExpr *expr);
static void check_if(Checker *self, AstIf *if_);
static void check_while(Checker *self, AstWhile *while_);

static void init_scoped_symbols(Checker *self);
static void free_scoped_symbols(Checker *self);
static void report_type_mismatch_error1(Report *self, const Type *want, const Type *have);

// Each AST node that is processed in a loop will use longjmp(buf, -1)
// to avoid having to process the error at after each function call
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

    // TODO:
    //  - seems redundant to store struct name in Type
    //  - we don't need to allocate Type
    Type *type = type_make_struct(struct_->name, &field_types, &field_names);
    append(&self->symbols->head, symbol_make_type(struct_->name, *type));
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

        append(&argument_types, symbol->type);
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

Type check_ident(Checker *self, const String *ident) {
    Symbol *symbol = symbol_find_with_kind(self->symbols, ident, VariableSymbol);
    if (symbol == nullptr) {
        report_error(self->report, "unknown identifier %.*s", STRING_FMT_ARGS(ident));
        longjmp(statement_jmp_buf, -1);
    }

    return symbol->type;
}

Type check_compount_ident(Checker *self, const Strings *idents) {
    assert(idents->len > 1);

    String base = idents->items[0];
    Symbol *symbol = symbol_find_with_kind(self->symbols, &base, VariableSymbol);
    if (symbol == nullptr) {
        report_error(self->report, "unknown identifier %.*s", STRING_FMT_ARGS(&base));
        longjmp(statement_jmp_buf, -1);
    }

    Type base_type = symbol->type;
    if (base_type.kind != StructType) {
        report_error(self->report, "cannot access fields of %.*s - not a struct", STRING_FMT_ARGS(&base));
        longjmp(statement_jmp_buf, -1);
    }


    // TODO: do I need to annotate the type for each ident? What about embedded structs?
    // Need to think about how to make this work nicely for code generation
    // An AstNode wrapper around the String could be better
    return base_type;
}

Type check_index(Checker *self, AstIndex *index) {
    Symbol *symbol = symbol_find_with_kind(self->symbols, &index->ident, VariableSymbol);
    if (symbol == nullptr) {
        report_error(self->report, "unknown identifier %.*s", STRING_FMT_ARGS(&index->ident));
        longjmp(statement_jmp_buf, -1);
    }

    Type *type = type_dereference(&symbol->type);
    if (type == nullptr) {
        report_error(self->report, "%.*s cannot be dereferenced", STRING_FMT_ARGS(&index->ident));
        longjmp(statement_jmp_buf, -1);
    }

    check_expr(self, &index->expr);
    if (!type_equal(&index->expr.node.type, &i64_type)) {
        report_error(self->report, "%.*s cannot be indexed by non-i64 type", STRING_FMT_ARGS(&index->ident));
        longjmp(statement_jmp_buf, -1);
    }

    return *type;
}

void check_location(Checker *self, AstLocation *location) {
    Type type = {0};

    switch (location->kind) {
    case LocationIdent:
        type = check_ident(self, &location->as.ident);
        break;
    case LocationCompoundIdent:
        type = check_compount_ident(self, &location->as.compound_ident);
        break;
    case LocationIndex:
        type = check_index(self, &location->as.index);
        break;
    }

    location->node.type = type;
    location->node.coercible = false;
}

void check_assign(Checker *self, AstAssign *assign) {
    check_location(self, &assign->location);

    check_expr(self, &assign->expr);

    if (!type_equal(&assign->location.node.type, &assign->expr.node.type)) {
        report_type_mismatch_error1(self->report, &assign->location.node.type, &assign->expr.node.type);
        longjmp(statement_jmp_buf, -1);
    }

    return;
}

void check_let(Checker *self, AstLet *let) {
    return;
}

void check_expr(Checker *self, AstExpr *expr) {
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
