#include "type2.h"
#include "ast.h"
#include "int.h"
#include "util.h"
#include "array.h"

typedef enum {
    FunctionSymbol,
    VariableSymbol,
    TypeSymbol,
} SymbolKind;

typedef struct {
    int local
} SymbolVariable;

typedef struct {
    u32 local;
} SymbolVariable;

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
        if (string_cmp(&symbol->key, key) == 0) {
            return symbol;
        }
    }

    return symbol_find(self->next, key);
}

Symbol symbol_make_variable(String name, Type type) {
    Symbol symbol = {0};
    symbol.name = name;
    symbol.kind = VariableSymbol;
    symbol.type = type;
    symbol.as.variable.local = 0; // Is this needed here?

    return symbol;
}

Symbol symbol_make_function(String key, Type type, Types argument_type) {
    Symbol symbol = {0};
    symbol.name = name;
    symbol.kind = FunctionSymbol;
    symbol.type = type;
    symbol.as.function.argument_types = argument_type;

    return symbol;
}

Symbol symbol_make_type(String key, Type type) {
    Symbol symbol = {0};
    symbol.name = name;
    symbol.kind = TypeSymbol;
    symbol.type = type;

    return symbol;
}

typedef struct {
    SymbolChain *symbols;
} Checker;

void init_scoped_symbols(Checker *self) {
    SymbolChain symbols = {0};
    SymbolChain *next = self->symbols;
    self->symbols = box(symbols);
    self->symbols->next = next;
}

void free_scoped_symbols(Checker *self) {
    SymbolChain *symbols = self->symbols->next;
    free(self->symbols);
    self->symbols = symbols;
}

void check_init(Checker *self) {
    append(&self->symbols->head, symbol_make_type(string_from_cstr("void"), void_type));
    append(&self->symbols->head, symbol_make_type(string_from_cstr("i8"), i8_type));
    append(&self->symbols->head, symbol_make_type(string_from_cstr("i32"), i32_type));
    append(&self->symbols->head, symbol_make_type(string_from_cstr("i64"), i64_type));
}

void check_file(Checker *self, const AstFile *file) {

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
        Symbol *symbol = symbol_find(self->symbols, function->return_type);
        if (symbol == nullptr) {
            TODO("symbol == nullptr");
        }
        if (symbol->kind != TypeSymbol) {
            TODO("symbol->kind != TypeSymbol");
        }

        return_type = symbol->type;
    }

    Types argument_types = {0};
    foreach(param, &function->params) {
        Symbol *symbol = symbol_find(self->symbols, &param->type.name);
        if (symbol == nullptr) {
            TODO("symbol == nullptr");
        }
        if (symbol->kind != TypeSymbol) {
            TODO("symbol->kind != TypeSymbol");
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
            TODO("redefinition of symbol");
        }

        append(&self->symbols->head, symbol_make_variable(param.name, argument_type));
    }

    // check block

    free_scoped_symbols(self);
}

void check_block(Checker *self, const AstBlock *block) {
    init_scoped_symbols(self);
    check_statements(self, &block->statements);
    free_scoped_symbols(self);
}

void check_statements(Checker *self, const AstStatements *statements) {
    foreach(statement, statements) {
        check_statement(self, statement);
    }
}

void check_statement(Checker *self, const AstStatement *statement) {
    switch (statement->kind) {
    case StatementAssign:
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
