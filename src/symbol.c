#include "symbol.h"
#include "array.h"
#include "str.h"

Symbol *symbol_find(SymbolChain *self, const String *key) {
    if (self ==  nullptr) return nullptr;

    foreach(symbol, &self->head) {
        if (string_cmp(&symbol->key, key) == 0) {
            return symbol;
        }
    }

    return symbol_find(self->next, key);
}

Symbol symbol_make_variable(String key, TypeID typeid, int local) {
    Symbol symbol = {0};
    symbol.key = key;
    symbol.kind = SymbolVariable;
    symbol.typeid = typeid;
    symbol.as.local = local;

    return symbol;
}

Symbol symbol_make_function(String key, TypeID typeid, TypeIDs args) {
    Symbol symbol = {0};
    symbol.key = key;
    symbol.kind = SymbolVariable;
    symbol.typeid = typeid;
    symbol.as.args = args;

    return symbol;
}
