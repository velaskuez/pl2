#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define ARRAY_CAPACITY 128

typedef struct {
    size_t len, cap;
} ArrayHeader;

#define append(array, item) {                                                                  \
    if ((array) == NULL) {                                                                     \
        ArrayHeader *header = malloc(sizeof(ArrayHeader) + (sizeof(*array) * ARRAY_CAPACITY)); \
        header->len = 0;                                                                       \
        header->cap = ARRAY_CAPACITY;                                                          \
        array = (typeof(*array) *)(header+1);                                                  \
    }                                                                                          \
    ArrayHeader *header = ((ArrayHeader *)array)-1;                                            \
    if (header->cap == header->len) {                                                          \
        header->cap *= 2;                                                                      \
        header = realloc(header, sizeof(ArrayHeader) + (sizeof(*array) * header->cap));        \
        assert(header != NULL);                                                                \
        array = (typeof(*array) *)(header+1);                                                  \
    }                                                                                          \
    (array)[header->len++] = (item);                                                           \
}

#define len(array) (array == NULL ? 0 : ((ArrayHeader *)array-1)->len)

#define foreach(item, array) \
    for (auto item = (array); item < (array) + len(array); item++)

// Look into doing something like this for deferred free
// #define VEC(type) __attribute__((cleanup(dynarr_free))) *type;

#define arrayfree(array) if (array) { free((ArrayHeader *)array-1); array = NULL; }
