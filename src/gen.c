#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "gen.h"
#include "array.h"
#include "ast.h"
#include "str.h"
#include "type.h"
#include "util.h"

void gen_init(Generator *self) {
    Type types[] = PRIMITIVE_TYPES;

    for (Type *type = &types[0]; type < type+sizeof(types); type++) {
        append(&self->types, *type);
    }
}

TypeID gen_type(Generator *self, const AstType *ast_type) {
    TypeID foundid = -1;

    for (size_t id = 0; id < self->types.len; id++) {
        Type *it = &self->types.items[id];

        if (string_cmp(&it->key, &ast_type->name) != 0) {
            continue;
        }

        foundid = id;
        if (ast_type->pointer != it->pointer) {
            // We don't want to create the pointer version of the type
            // here since we may find it later in this loop.
            continue;
        }

        return foundid;
    }

    if (foundid < 0) return -1;

    // The type exists but it is not ast_type->pointer
    // It shouldn't be possible for the pointer type to be defined before
    // the actual type.
    assert(ast_type->pointer);
    Type type = self->types.items[foundid];
    type.pointer = true;
    type.size = 8;
    type.slotsize = DoubleSlot;
    type.alignment = 8;
    append(&self->types, type);

    return self->types.len-1;
}

void gen_struct(Generator *self, const AstStruct *ast_struct) {
    Struct struct_ = {0};

    foreach(ast_field, &ast_struct->fields) {
        TypeID typeid = gen_type(self, &ast_field->type);
        if (typeid < 0) {
            TODO("handle unknown type");
        }

        // Ensure any fields which are structs are also pointers
        // since we're compiling for stack
        if (typeid > I8PtrTypeID && !ast_field->type.pointer) {
            TODO("structs must only contain pointers to other structs");
        }

        Type type = self->types.items[typeid];

        // We need to resolve all of the field types before we can calculate
        // the offsets of them
        StructField field = {0};
        field.id = typeid;
        // field.offset = later
        append(&struct_.field_types, field);
    }

    size_t offset = 0;
    size_t padding = 0;
    for (size_t i = 0; i < &struct_.field_types.len; i++) {
        StructField *it = &struct_.field_types.items[i];

        it->offset = offset;
        offset += self->types.items[it->id].size;

        // If padding < 8, then we may have room to add in the next field
        padding = 8 - (offset % 8);
        if (padding == 0) {
            continue;
        }

        if (i == struct_.field_types.len-1) {
            // Last element - the size of the struct will be a mulitple of 8
            offset += padding;
            break;
        }

        // If the next type has the same alignment, then continue
        // Otherwise, add padding
        StructField *next = &struct_.field_types.items[i+1];
        if (self->types.items[it->id].alignment == self->types.items[next->id].alignment) {
            continue;
        }

        offset += padding;
    }

    // At this point, offset = size
    Type type = {0};
    type.key = ast_struct->name;
    type.realsize = type.size = offset;
    type.pointer = false;
    type.alignment = 8;
    type.slotsize = Invalid;
    append(&self->types, type);

    struct_.id = self->types.len-1;

    return;
}
