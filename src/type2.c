#include "type2.h"

Type void_type = {
    .kind = PrimitiveType,
    .layout.alignment = 0,
    .layout.size = 0,
    .as.primitive.kind = PrimitiveVoid
};

Type i8_type = {
    .kind = PrimitiveType,
    .layout.alignment = 1,
    .layout.size = 1,
    .as.primitive.kind = PrimitiveI8
};

Type i32_type = {
    .kind = PrimitiveType,
    .layout.alignment = 4,
    .layout.size = 4,
    .as.primitive.kind = PrimitiveI32
};

Type i64_type = {
    .kind = PrimitiveType,
    .layout.alignment = 8,
    .layout.size = 8,
    .as.primitive.kind = PrimitiveI64
};

bool type_equal(const Type *t, const Type *u) {
    if (t->kind != u->kind) return false;

    switch (t->kind) {
    case PrimitiveType:
        return type_equal_primitive(&t->as.primitive, &u->as.primitive);
    case PointerType:
        return type_equal_pointer(&t->as.pointer, &u->as.pointer);
    case StructType:
        return type_equal_struct(&t->as.struct_, &u->as.struct_);
    case ArrayType:
        return type_equal_array(&t->as.array, &u->as.array);
    }
}

bool type_equal_primitive(const TypePrimitive *t, const TypePrimitive *u) {
    return t->kind == u->kind;
}

bool type_equal_pointer(const TypePointer *t, const TypePointer *u) {
    return type_equal(t->type, u->type);
}

bool type_equal_struct(const TypeStruct *t, const TypeStruct *u) {
    return string_cmp(&t->name, &u->name) == 0;
}

bool type_equal_array(const TypeArray *t, const TypeArray *u) {
    return t->length == u->length && type_equal(t->type, u->type);
}

bool type_coerce(const Type *from, const Type *to) {
    if (from->kind != PrimitiveType || to->kind != PrimitiveType) {
        return false;
    }

    if (from->layout.size == 0 || to->layout.size == 0) {
        return false;
    }

    return from->layout.size <= to->layout.size;
}

bool type_cast(const Type *from, const Type *to) {
    switch (from->kind) {
    case PrimitiveType:
        return to->kind == PrimitiveType;
    case PointerType:
        return to->kind == PointerType ||
               to->kind == PrimitiveType ||
               (to->kind == ArrayType && type_equal(from->as.pointer.type, to->as.array.type));
    case StructType:
        return type_equal(from, to);
    case ArrayType:
        return type_equal(from, to) ||
               (to->kind == PointerType && type_equal(from->as.array.type, to->as.pointer.type));
    }
}

Type *type_dereference(const Type *from) {
    switch (from->kind) {
    case PrimitiveType:
    case StructType:
        return nullptr;
    case PointerType:
        return from->as.pointer.type;
    case ArrayType:
        return from->as.array.type;
    }
}

Type *type_make_pointer(Type *from) {
    Type *type = calloc(1, sizeof(Type));
    type->kind = PointerType;
    type->layout = i64_type.layout;
    type->as.pointer.type = from;

    return type;
}

Type *type_make_array(Type *from, u32 length) {
    Type *type = calloc(1, sizeof(Type));
    type->kind = ArrayType;
    type->layout.size = from->layout.size * length;
    type->layout.alignment = from->layout.alignment;
    type->as.array.length = length;
    type->as.array.type = from;

    return type;
}

Type *type_make_struct(String name, const Types *types, const Strings *names) {
    Type *type = calloc(1, sizeof(Type));
    type->kind = StructType;

    TypeLayout layout = {0};
    layout.alignment = 8;

    TypeStruct struct_ = {0};
    struct_.name = name;

    {size_t i = 0;
    for (const Type *field_type = types->items; field_type < types->items + types->len; field_type++, i++) {
        u32 offset = layout.size;
        u32 size = type->layout.size;
        u32 padding = 0;

        TypeStructField field = {0};
        field.offset = offset;
        field.type = type;

        // If padding < 8, then we may have room to add in the next field
        padding = 8 - (offset % 8);
        if (padding == 0 || padding == 8) {
            continue;
        }

        if (i == types->len-1) {
            // Last element - add padding so the size of the struct
            // will be a mulitple of 8
            layout.size += size;
            break;
        }

        // Two fields can be directly adjacent if either
        //  - They both have the same alignment
        //  - The first field is placed so that the next field will
        //    be correctly aligned
        size_t field_alignment = type->layout.alignment;
        size_t next_field_alignment = types->items[i+1].layout.alignment;
        if (field_alignment == next_field_alignment ||
                (field_alignment % offset == 0 && next_field_alignment < field_alignment)) {
            continue;
        }

        layout.size += size;
    }}

    type->layout = layout;
    type->as.struct_ = struct_;

    return type;
}

char *type_fmt(const Type *self) {
    return "TODO";
}
