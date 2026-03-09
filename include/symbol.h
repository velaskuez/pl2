#pragma once

#include "type.h"
#include "str.h"
#include "type.h"

// Type names could also be thought of as symbols, but during generation
// we'll look at the types array for them, not the symbols array. Which also
// means users can use type names as variable names.
// Similar with function calls, however we might later decide to support passing
// around function pointers, so we'll keep this as a tagged union to make
// implementing that easier.
typedef enum {
    SymbolFunction,
    SymbolVariable,
} SymbolKind;

typedef struct {
    SymbolKind kind;
    String key;
    Type type;
    union {
        Types arg_types;
        int local;
    } as;
} Symbol;

typedef struct {
    size_t len, cap;
    Symbol *items;
} Symbols;

typedef struct SymbolChain SymbolChain;
struct SymbolChain{
    Symbols head;
    SymbolChain *next;
};

Symbol *symbol_find(const SymbolChain *, const String *key);

Symbol symbol_make_variable(String key, Type type, int local);
Symbol symbol_make_function(String key, Type type, Types args);
