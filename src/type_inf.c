#include <stdbool.h>

#include "str.h"
#include "type.h"
#include "type_inf.h"
#include "ast.h"
#include "symbol.h"
#include "util.h"

static void infer_type_inner(TypeInf *self, const AstExpr *expr);
static void infer_binary_op_type(TypeInf *self, const AstBinaryOp *binary_op);
static void infer_unary_op_type(TypeInf *self, const AstUnaryOp *unary_op);
static void infer_value_type(TypeInf *self, const AstValue *value);
static void infer_ident_type(TypeInf *self, const String *ident);
static void infer_compound_ident_type(TypeInf *self, const Strings *idents);
static void infer_call_type(TypeInf *self, const AstCall *call);
static void handle_known_type(TypeInf *self, const Type *type);
static bool types_match(const Type *t, const Type *u);

// TODO: reporter for compiler errors which can be invoked outside of gen.c

const Type* infer_type(const Types *types, const Structs *structs, const SymbolChain *symbols,
                 const AstExpr *expr) {
    TypeInf inf = {0};
    inf.coercible = false;
    inf.types = types;
    inf.structs = structs;
    inf.symbols = symbols;

    infer_type_inner(&inf, expr);

    return inf.inferred_type;
}

// An expression of integer literals may evaluate to i32 by default,
// but a let binding may specify the type as a i8 or i64, which is permitted
// Some coercions aren't permitted such as struct to different struct, or pointer to non-pointer
bool can_coerce_types(const Type *t, const Type *u) {
    if (types_match(t, u)) return true;

    if (t->struct_ || u->struct_) return false;
    if (t->pointer != u->pointer) return false;

    return true;
}

void infer_type_inner(TypeInf *self, const AstExpr *expr) {
    switch (expr->kind) {
    case ExprBinaryOp:
        infer_binary_op_type(self, &expr->as.binary_op);
        break;
    case ExprUnaryOp:
        infer_unary_op_type(self, &expr->as.unary_op);
        break;
    case ExprValue:
        infer_value_type(self, &expr->as.value);
        break;
    case ExprIdent:
        infer_ident_type(self, &expr->as.ident);
        break;
    case ExprCompoundIdent:
        infer_compound_ident_type(self, &expr->as.compound_ident);
        break;
    case ExprCall:
        infer_call_type(self, &expr->as.call);
        break;
    }
}

void infer_binary_op_type(TypeInf *self, const AstBinaryOp *binary_op) {
    infer_type_inner(self, binary_op->left);
    infer_type_inner(self, binary_op->right);
}

void infer_unary_op_type(TypeInf *self, const AstUnaryOp *unary_op) {
    infer_type_inner(self, unary_op->expr);
}

void infer_value_type(TypeInf *self, const AstValue *value) {
    Type *type = nullptr;
    switch (value->kind) {
        case ValueNumber:
            type = &self->types->items[I32TypeID];
            break;
        case ValueString:
            type = &self->types->items[I8PtrTypeID];
            break;
        case ValueChar:
            type = &self->types->items[I8TypeID];
            break;
    }

    if (self->inferred_type == nullptr) {
        // leaving set_type as true in case we encounter
        // any symbols with associated types
        self->inferred_type = type;

        // If inferred_type is null, then we haven't visited any identifiers
        // or calls yet. If we do, we'll prefer coercing inferred_type
        // rather than matching it exactly
        self->coercible = true;

        return;
    }

    // Since the inferred_type is already set, we just need
    // to ensure the literal's type matches or can at least be
    // coerced.
    if (!can_coerce_types(self->inferred_type, type)) {
        TODO("literal coercion error handling");
    }
}

void infer_ident_type(TypeInf *self, const String *ident) {
    Symbol *symbol = symbol_find(self->symbols, ident);
    if (symbol == nullptr) {
        TODO("unknown symbol error handling");
    }

    Type *type = &self->types->items[symbol->typeid];

    handle_known_type(self, type);
}

void infer_compound_ident_type(TypeInf *self, const Strings *idents) {
    TODO("infer_compound_ident_type");
}

void infer_call_type(TypeInf *self, const AstCall *call) {
    Symbol *symbol = symbol_find(self->symbols, &call->name);
    if (symbol == nullptr) {
        TODO("unknown symbol error handling");
    }

    Type *type = &self->types->items[symbol->typeid];

    handle_known_type(self, type);
}

void handle_known_type(TypeInf *self, const Type *type) {
    if (self->inferred_type == nullptr) {
        self->coercible = false;
        self->inferred_type = type;

        return;
    }

    if (!self->coercible) {
        if (!types_match(self->inferred_type, type)) {
            TODO("ident type match error handling");
        }

        return;
    }

    if (!can_coerce_types(self->inferred_type, type)) {
        TODO("ident coercion error handling");
    }

    self->coercible = false;
    self->inferred_type = type;
}

bool types_match(const Type *t, const Type *u) {
    if (string_cmp(&t->key, &u->key) != 0) return false;
    if (t->pointer != u->pointer) return false;

    return true;
}
