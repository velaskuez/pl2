#pragma once

#include "array.h"

typedef struct {
    int errors;
    int warnings;
} Report;

void report_error(Report *self, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void report_internal_error(Report *self, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
void report_warning(Report *self, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
