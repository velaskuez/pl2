#pragma once

#include "ast.h"
#include "type.h"
#include "symbol.h"
#include "report.h"

typedef struct {
    const Types *types;
    const Structs *structs;
    const SymbolChain *symbols;
    // If inferred_type isn't null, we need to check types match or
    // can be coerced depending on `coercible`
    const Type *inferred_type;
    Report *report;
    bool coercible;
} TypeInf;

typedef struct {
    const Type *type;
    bool is_coercible;
} Inferred;

Inferred type_infer(Report *report,
                    const Types *types, const Structs *structs,
                    const SymbolChain *symbols, const AstExpr *expr);

bool type_match(const Type *t, const Type *u);
bool type_coercible(const Type *t, const Type *u);
bool type_match_or_coercible(const Inferred *inferred, const Type *u);
