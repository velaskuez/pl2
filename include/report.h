#pragma once

#include "type.h"
#include "array.h"

typedef struct {
    int errors;
    int warnings;
} Report;

void report_error(Report *self, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void report_internal_error(Report *self, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void report_warning(Report *self, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

void report_type_mismatch_error(Report *self, const Type *want, const Type *have);
