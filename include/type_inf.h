#pragma once

#include "ast.h"
#include "type.h"
#include "symbol.h"

TypeID infer_type(Types *self, SymbolChain* symbols, const AstExpr *expr);
bool can_coerce_types(Types *self, TypeID t, TypeID u);
