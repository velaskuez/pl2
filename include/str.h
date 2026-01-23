#pragma once

#include <stdlib.h>

typedef struct {
    size_t len;
    size_t cap;
    char *items;
} String;

#define EMPTY_STRING (String){0}

String string_from_file(int);
String string_from_cstr(char *);
