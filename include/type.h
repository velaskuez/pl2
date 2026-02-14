#pragma once

#include "str.h"

typedef enum {
    Invalid = 0,
    SingleSlot,
    DoubleSlot,
} SlotSize;

typedef size_t TypeID ; // The index into the Types array
typedef enum TypeID {
    VoidTypeID,
    I8TypeID,
    I32TypeID,
    I64TypeID,
    I8PtrTypeID // If typeid > I8PtrTypeID then it is a user defined type
} PrimitiveTypeID;

typedef struct {
    String key;
    bool pointer;
    // For primitive types (int, long, char), size = realsize
    // If type is a pointer, size = 8, realsize = the size to allocate
    int size, realsize;
    SlotSize slotsize;
    int alignment;
} Type;

typedef struct {
    size_t len, cap;
    Type *items;
} Types;

typedef struct {
    TypeID id;     // Index into the types array
    size_t offset; // The offset from the base pointer
} StructField;

typedef struct {
    size_t len, cap;
    StructField *items;
} StructFields;

typedef struct {
    TypeID id;                // Index into the types array
    StructFields field_types; // Index into the types array for each field
} Struct;

typedef struct {
    size_t len, cap;
    Struct *items;
} Structs;

#define PRIMITIVE_TYPES \
    { \
        [VoidTypeID] = { \
            .key = string_from_cstr("void"), \
            .pointer = false, \
            .size = 0, \
            .realsize = 0, \
            .slotsize = Invalid, \
            .alignment = 0, \
        }, \
        [I8TypeID] = { \
            .key = string_from_cstr("i8"), \
            .pointer = false, \
            .size = 1, \
            .realsize = 1, \
            .slotsize = SingleSlot, \
            .alignment = 1, \
        }, \
        [I32TypeID] = { \
            .key = string_from_cstr("i32"), \
            .pointer = false, \
            .size = 4, \
            .realsize = 4, \
            .slotsize = SingleSlot, \
            .alignment = 4, \
        }, \
        [I64TypeID] = { \
            .key = string_from_cstr("i64"), \
            .pointer = false, \
            .size = 8, \
            .realsize = 8, \
            .slotsize = DoubleSlot, \
            .alignment = 8, \
        }, \
        [I8PtrTypeID] = { \
            .key = string_from_cstr("i8"), \
            .pointer = true, \
            .size = 8, \
            .realsize = 8, \
            .slotsize = DoubleSlot, \
            .alignment = 8, \
        } \
    } \

