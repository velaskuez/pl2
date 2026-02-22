#pragma once

#include <stdio.h>
#include "string.h"

#define box(data) ({                           \
    auto value = data;                         \
    void *allocation = malloc(sizeof(value));  \
    memcpy(allocation, &value, sizeof(value)); \
    allocation;                                \
})

// "if the variable arguments are omitted or empty, the ‘##’ (token paste)
// operator causes the preprocessor to remove the comma before it"
#define panic(msg, ...) { \
    fprintf(stderr, "%s:%d: "msg"\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    exit(1); \
}

#define TODO(msg) \
    panic("%s: "msg, "TODO")

