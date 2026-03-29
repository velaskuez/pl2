#pragma once

#include "symbol2.h"
#include "report.h"
#include "ast.h"

typedef struct {
    SymbolChain *symbols;
    Report *report;
    Type current_function_return_type;
    bool current_function_returns;
} Checker;

void check_init(Checker *self, Report *report);
void check_file(Checker *self, const AstFile *file);
