#pragma once

#include "str.h"
#include "int.h"
#include "report.h"
#include "ast.h"

typedef struct {
    String name;
    i32 local;
} Variable;

typedef struct {
    size_t cap, len;
    Variable *items;
} Variables;

typedef struct VariableChain VariableChain;
struct VariableChain {
    Variables head;
    VariableChain *next;
};

typedef struct {
    VariableChain *variables;

    int local; // For allocating temporary variables
    int string; // For assigning labels to static strings
    int label; // For allocating labels for control flow

    char *ret_ext;

    int (*write_fn)(const char *, ...) __attribute__((format(printf, 1, 2)));

    Report *report;
} Generator;

void gen_init(Generator *self, Report *report);
void gen_file(Generator *self, const AstFile *file);
