#pragma once

#include "type.h"
#include "symbol.h"

typedef struct {
    Types types;
    SymbolChain *symbols;
    // int (*write_fn)(const char *, ...);
} Generator;
