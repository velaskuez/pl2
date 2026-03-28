#include "type2.h"
#include "symbol2.h"
#include "array.h"

char *symbol_str[TypeSymbol+1] = {
    [FunctionSymbol] = "function",
    [VariableSymbol] = "variable",
    [TypeSymbol] = "type"
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
