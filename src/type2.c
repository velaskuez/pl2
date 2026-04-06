#include "type2.h"
#include "array.h"
#include "ast.h"

char *primitive_type_str[PrimitiveI64+1] = {
    [PrimitiveVoid] = "void",
    [PrimitiveI8] = "i8",
    [PrimitiveI32] = "i32",
    [PrimitiveI64] = "i64"
};

char *type_kind_str[ArrayType+1] = {
    [PrimitiveType] = "primitive",
    [PointerType] = "pointer",
    [StructType] = "struct",
    [ArrayType] = "array"
};

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
        return (to->kind == PrimitiveType && to->as.primitive.kind != PrimitiveVoid) ||
            to->kind == PointerType;
    case PointerType:
        return to->kind == PointerType ||
               (to->kind == PrimitiveType && to->as.primitive.kind == PrimitiveI64) ||
               to->kind == ArrayType;
    case StructType:
        return type_equal(from, to);
    case ArrayType:
        return type_equal(from, to) ||
               to->kind == PointerType;
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

Type type_make_pointer(Type *from) {
    Type type = {0};
    type.kind = PointerType;
    type.layout = i64_type.layout;
    type.as.pointer.type = calloc(1, sizeof(Type));
    *type.as.pointer.type = *from;

    return type;
}

Type type_make_array(Type *from, u32 length) {
    Type type = {0};
    type.kind = ArrayType;
    type.layout.size = from->layout.size * length;
    type.layout.alignment = from->layout.alignment;
    type.as.array.length = length;
    type.as.array.type = calloc(1, sizeof(Type));
    *type.as.array.type = *from;

    return type;
}

Type type_make_struct(String name, const Types *types, const Strings *names) {
    Type type = {0};
    type.kind = StructType;

    TypeStruct struct_ = {0};
    struct_.name = name;

    u32 offset = 0;
    u64 i = 0;
    for (const Type *field_type = types->items;
                     field_type < types->items + types->len;
                     field_type++, i++) {

        TypeStructField field = {0};
        field.name = names->items[i];
        field.offset = offset;
        field.type = calloc(1, sizeof(Type));
        *field.type = *field_type;
        append(&struct_.fields, field);

        offset += field_type->layout.size;

        // If padding < 8, then we may have room to add in the next field
        u32 padding = 8 - (offset % 8);
        if (padding == 0 || padding == 8) {
            continue;
        }

        if (i == types->len-1) {
            // Last element - add padding so the size of the struct
            // will be a mulitple of 8
            offset += padding;
            break;
        }

        // Two fields can be directly adjacent if either
        //  - They both have the same alignment
        //  - The first field is placed so that the next field will
        //    be correctly aligned
        size_t field_alignment = field_type->layout.alignment;
        size_t next_field_alignment = types->items[i+1].layout.alignment;
        if (field_alignment == next_field_alignment ||
                (offset % next_field_alignment == 0)) {
            continue;
        }

        offset += padding;
    }

    TypeLayout layout = {0};
    layout.alignment = 8;
    layout.size = offset;

    type.layout = layout;
    type.as.struct_ = struct_;

    return type;
}

TypeStructField* struct_find_field(const TypeStruct *struct_, const String *name) {
    foreach(field, &struct_->fields) {
        if (string_cmp(&field->name, name) == 0) return field;
    }

    return nullptr;
}

char *type_fmt(const Type *self) {
    return "TODO";
}

void type_fprint(FILE *fp, const Type *self) {
    switch (self->kind) {
    case PrimitiveType:
        fprintf(fp, "%s", primitive_type_str[self->as.primitive.kind]);
        break;
    case PointerType:
        fprintf(fp, "*");
        type_fprint(fp, self->as.pointer.type);
        break;
    case StructType:
        fprintf(fp, "%.*s", STRING_FMT_ARGS(&self->as.struct_.name));
        break;
    case ArrayType:
        fprintf(fp, "[%d]", self->as.array.length);
        type_fprint(fp, self->as.array.type);
        break;
    }
}
