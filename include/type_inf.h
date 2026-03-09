#pragma once

#include "ast.h"
#include "type.h"
#include "symbol.h"

typedef struct {
    const Types *types;
    const Structs *structs;
    const SymbolChain *symbols;
    // If inferred_type isn't null, we need to check types match or
    // can be coerced depending on `coercible`
    const Type *inferred_type;
    bool coercible;
} TypeInf;

typedef struct {
    const Type *type;
    bool is_coercible;
} Inferred;

Inferred infer_type(const Types *types, const Structs *structs, const SymbolChain *symbols,
                              const AstExpr *expr);
bool can_coerce_types(const Type *t, const Type *u);
bool types_match(const Type *t, const Type *u);
