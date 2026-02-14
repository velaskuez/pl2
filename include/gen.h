#pragma once

#include "type.h"

typedef struct {
    Types types;
    // int (*write_fn)(const char *, ...);
} Generator;
