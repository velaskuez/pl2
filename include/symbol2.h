#pragma once

#include "type2.h"

typedef enum {
    FunctionSymbol,
    VariableSymbol,
    TypeSymbol,
} SymbolKind;

char *symbol_str[TypeSymbol+1];

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

Symbol *symbol_find(const SymbolChain *self, const String *key);
Symbol *symbol_find_with_kind(const SymbolChain *self, const String *key, SymbolKind kind);
Symbol symbol_make_variable(String name, Type type);
Symbol symbol_make_function(String name, Type type, Types argument_type);
Symbol symbol_make_type(String name, Type type);
