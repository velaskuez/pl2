#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "int.h"
#include "str.h"

typedef struct Type Type;

typedef enum {
    PrimitiveType, // coercible, castable -> primitive
    PointerType, // castable -> pointer, array, primitive
    StructType, // concrete
    ArrayType  // castable -> pointer
} TypeKind;

char *type_kind_str[ArrayType+1];

typedef struct {
    u32 size;
    u32 alignment;
} TypeLayout;

typedef struct {
    String name;
    Type *type;
    u32 offset;
} TypeStructField;

typedef struct {
    size_t len, cap;
    TypeStructField *items;
} TypeStructFields;

typedef struct {
    String name;
    TypeStructFields fields;
} TypeStruct;

typedef struct {
    Type *type;
    u32 length;
} TypeArray;

typedef struct {
    Type *type;
} TypePointer;

typedef enum {
    PrimitiveVoid,
    PrimitiveI8,
    PrimitiveI32,
    PrimitiveI64,
} TypePrimitiveKind;

char *primitive_type_str[PrimitiveI64+1];

typedef struct {
    TypePrimitiveKind kind;
} TypePrimitive;

struct Type {
    TypeKind kind;
    TypeLayout layout;
    union {
        TypePrimitive primitive;
        TypePointer pointer;
        TypeStruct struct_;
        TypeArray array;
    } as;
};

typedef struct {
    size_t len, cap;
    Type *items;
} Types;

Type void_type, i8_type, i32_type, i64_type;

bool type_equal(const Type *t, const Type *u);
bool type_equal_primitive(const TypePrimitive *t, const TypePrimitive *u);
bool type_equal_pointer(const TypePointer *t, const TypePointer *u);
bool type_equal_struct(const TypeStruct *t, const TypeStruct *u);
bool type_equal_array(const TypeArray *t, const TypeArray *u);

bool type_coerce(const Type *from, const Type *to);
bool type_cast(const Type *from, const Type *to);

Type *type_dereference(const Type *from);

Type type_make_pointer(Type *from);
Type type_make_array(Type *from, u32 length);
Type type_make_struct(String name, const Types *types, const Strings *names);

TypeStructField* struct_find_field(const TypeStruct *struct_, const String *name);

char *type_fmt(const Type *self);
void type_fprint(FILE *fp, const Type *self);
