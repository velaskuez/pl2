#include "type.h"
#include "util.h"

const char *op_ext(const Type *type) {
    // TODO: .b exists

    switch (type->slotsize) {
    case Invalid:
        panic("attempt to call op_ext() on unsized type");
    case SingleSlot:
        return ".w";
    case DoubleSlot:
        return ".d";
    default:
        panic("unreachable");
    }
}

const char *ret_ext(const Type *type) {
    switch (type->slotsize) {
    case Invalid:
        return "";
    case SingleSlot:
        return ".w";
    case DoubleSlot:
        return ".d";
    }
}
