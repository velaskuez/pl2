#pragma once

#include <stdlib.h>

#include "array.h"
#include "str.h"

// djb2 - http://www.cse.yorku.ca/~oz/hash.html
size_t hash(const String *s) {
    unsigned long hash = 5381;

    for (size_t i = 0; i < s->len; i++) {
        hash = ((hash << 5) + hash) + s->items[i]; /* hash * 33 + c */
    }

    return hash;
}

#define insert(map, item)                                                                          \
    ({                                                                                             \
        if ((map)->cap == 0) {                                                                     \
            (map)->cap = 128;                                                                      \
            (map)->items = calloc((map)->cap, sizeof((map)->items[0]));                            \
        }                                                                                          \
                                                                                                   \
        size_t i = hash(&item.key) % 128;                                                          \
        auto bucket = &(map)->items[i];                                                            \
                                                                                                   \
        bool found = false;                                                                        \
        for (size_t i = 0; i < bucket->len; i++) {                                                 \
            if (stringcmp(&bucket->items[i].key, &item.key) == 0) {                                \
                bucket->items[i] = item;                                                           \
                found = true;                                                                      \
                break;                                                                             \
            }                                                                                      \
        }                                                                                          \
                                                                                                   \
        if (!found) {                                                                              \
            append(bucket, item);                                                                  \
        }                                                                                          \
    })

#define get(map, search)                                                                           \
    ({                                                                                             \
        typeof((map)->items[0].items[0]) *value = nullptr;                                         \
                                                                                                   \
        if ((map)->cap == 0) {                                                                     \
            value = nullptr;                                                                       \
        } else {                                                                                   \
            size_t i = hash(search) % 128;                                                         \
            auto bucket = &(map)->items[i];                                                        \
                                                                                                   \
            for (size_t i = 0; i < bucket->len; i++) {                                             \
                if (stringcmp(&bucket->items[i].key, search) == 0) {                               \
                    value = &bucket->items[i];                                                     \
                }                                                                                  \
            }                                                                                      \
        }                                                                                          \
                                                                                                   \
        value;                                                                                     \
    })

#define clear(map)                                                                                 \
    for (size_t i = 0; i < (map)->cap; i++) {                                                      \
        (map)->items[i].len = 0;                                                                   \
    }

#define mapfree(map)                                                                               \
    if ((map)->cap != 0) {                                                                         \
        for (size_t i = 0; i < (map)->cap; i++) {                                                  \
            arrayfree((map)->items[i]);                                                            \
        }                                                                                          \
        free((map)->items));                                                                       \
        (map)->items = nullptr;                                                                    \
    }\
