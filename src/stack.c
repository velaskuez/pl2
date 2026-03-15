#include "type.h"
#include "util.h"

const char *op_ext(const Type *type) {
    if (type->size == 1) {
        return ".b";
    } else if (type->size > 1 && type->size <= 4) {
        return ".w";
    } else if (type->size > 4 && type->size <= 8) {
        return ".d";
    } else if (type->struct_id > 0) {
        panic("attempt to call op_ext() on struct");
    } else if (type->size == 0) {
        panic("attempt to call op_ext() on unsized type");
    }

    panic("unreachable");
}

const char *ret_ext(const Type *type) {
    if (type->size == 0) {
        return "";
    } else if (type->size >= 1 && type->size <= 4) {
        return ".w";
    } else if (type->size > 4 && type->size <= 8) {
        return ".d";
    } else if (type->struct_id > 0) {
        panic("attempt to call ret_ext() on struct");
    }

    panic("unreachable");
}
