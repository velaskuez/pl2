#include <stdio.h>
#include <stdarg.h>

#include "report.h"
#include "array.h"
#include "util.h"

void report_error(Report *self, const char *fmt, ...) {
    self->errors += 1;

    fprintf(stderr, "ERROR: ");

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
}

void report_internal_error(Report *self, const char *fmt, ...) {
    self->errors += 1;

    fprintf(stderr, "ERROR: ");

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
    fprintf(stderr, "(This is a bug in the compiler)\n");
}

void report_warning(Report *self, const char *fmt, ...) {
    self->warnings += 1;

    fprintf(stderr, "WARNING: ");

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
}
