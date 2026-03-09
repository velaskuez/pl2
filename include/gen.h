#pragma once

#include "type.h"
#include "symbol.h"
#include "ast.h"

typedef struct {
    Types types;
    Structs structs;
    SymbolChain *symbols;

    int var; // For allocating temporary variables
    int string; // For assigning labels to static strings

    int (*write_fn)(const char *, ...) __attribute__((format(printf, 1, 2)));
} Generator;


void gen_init(Generator *self);
void gen_file(Generator *self, const AstFile *file);
