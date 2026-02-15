#pragma once

#define ARRAY_CAP 128
#define ARRAY_MUL 2

#define append(array, item)                                                                        \
    if ((array)->cap == 0) {                                                                       \
        (array)->cap = ARRAY_CAP;                                                                  \
    }                                                                                              \
    if ((array)->len == (array)->cap) {                                                            \
        (array)->cap *= ARRAY_MUL;                                                                 \
    }                                                                                              \
    (array)->items = realloc((array)->items, (array)->cap * sizeof((array)->items[0]));            \
    (array)->items[(array)->len++] = item;

#define arrayfree(array)                                                                           \
    if ((array)->items != nullptr) {                                                               \
        free((array)->items);                                                                      \
        (array)->items = nullptr;                                                                  \
    }\

#define foreach(item, array) \
    for (auto item = (array)->items; item < (array)->items + (array)->len; item++)

#define upto(len) \
    for (auto i = 0; i < len; i++)
