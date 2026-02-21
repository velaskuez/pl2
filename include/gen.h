#pragma once

#include "type.h"
#include "symbol.h"
#include "ast.h"

typedef struct {
    Types types;
    SymbolChain *symbols;

    int var;

    int (*write_fn)(const char *, ...);
} Generator;


void gen_init(Generator *self);
void gen_file(Generator *self, const AstFile *file);
