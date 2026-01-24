#pragma once

#include <stdlib.h>

typedef struct {
    size_t len, cap;
    char *items;
} String;

typedef struct {
    size_t len, cap;
    String *items;
} Strings;

#define EMPTY_STRING (String){0}

String string_from_file(int);
String string_from_cstr(char *);
int string_cstr_cmp(const String *, char *);
