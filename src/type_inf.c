#include <stdbool.h>
#include <assert.h>

#include "str.h"
#include "type.h"
#include "type_inf.h"
#include "ast.h"
#include "symbol.h"
#include "util.h"
#include "array.h"

static void type_infer_inner(TypeInf *self, const AstExpr *expr);
static void infer_binary_op_type(TypeInf *self, const AstBinaryOp *binary_op);
static void infer_unary_op_type(TypeInf *self, const AstUnaryOp *unary_op);
static void infer_value_type(TypeInf *self, const AstValue *value);
static void infer_ident_type(TypeInf *self, const String *ident);
static void infer_compound_ident_type(TypeInf *self, const Strings *idents);
static void infer_call_type(TypeInf *self, const AstCall *call);
static void handle_known_type(TypeInf *self, const Type *type);

// TODO: reporter for compiler errors which can be invoked outside of gen.c

// TODO: type_infer can either return a pointer to a type in the types array or a symbol type
// which is messy. This makes me think that the typeid approach is probably more consistent
// even though it requires a few extra lines of code whereever it's used. It's also safer in case
// of reallocations and requires less space than storing the Type struct or possibly
// even a pointer to it.
Inferred type_infer(const Types *types, const Structs *structs, const SymbolChain *symbols,
                       const AstExpr *expr) {
    TypeInf self = {0};
    self.coercible = false;
    self.types = types;
    self.structs = structs;
    self.symbols = symbols;

    type_infer_inner(&self, expr);

    Inferred inferred_type = {0};
    inferred_type.type = self.inferred_type;
    inferred_type.is_coercible = self.coercible;

    return inferred_type;
}

bool type_match(const Type *t, const Type *u) {
    if (string_cmp(&t->key, &u->key) != 0) return false;
    if (t->pointer != u->pointer) return false;

    return true;
}

bool type_coercible(const Type *t, const Type *u) {
    // Can T be coerced to U?
    if (t->struct_id > 0 || u->struct_id > 0) return false;
    if (t->pointer != u->pointer) return false;
    if (t->realsize > u->realsize) return false;

    return true;
}

bool type_match_or_coercible(const Inferred *inferred, const Type *u) {
    const Type *t = inferred->type;
    assert(t != nullptr);

    if (type_match(t, u)) return true;

    if (!inferred->is_coercible) return false;

    return type_coercible(inferred->type, u);
}

void type_infer_inner(TypeInf *self, const AstExpr *expr) {
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
    type_infer_inner(self, binary_op->left);
    type_infer_inner(self, binary_op->right);
}

void infer_unary_op_type(TypeInf *self, const AstUnaryOp *unary_op) {
    Type *type = nullptr;

    switch (unary_op->op) {
        case UnaryOpNew:
            if (unary_op->expr->kind != ExprIdent) {
                TODO("report error");
                return;
            }

            foreach(it, self->types) {
                if (string_cmp(&it->key, &unary_op->expr->as.ident) != 0) continue;
                if (!it->pointer) continue;

                type = it;
                break;
            }

            if (type == nullptr) {
                TODO("report error");
                return;
            }

            break;
        case UnaryOpSizeOf:
            type = &self->types->items[I64TypeID];
            break;
        default:
            panic("unreachable");
    }

    handle_known_type(self, type);
}

void infer_value_type(TypeInf *self, const AstValue *value) {
    Type *type = nullptr;
    switch (value->kind) {
        case ValueNumber:
            // TODO: It would be better to store the sign as it's own
            // flag rather than using i64 for everything

            // Use the smallest possible type so that it can be coerced
            // to larger types if necessary.
            i64 number = value->as.number;
            if (number >= -128 && number <= 127) {
                type = &self->types->items[I8TypeID];
            } else if (number >= -2'147'438'648 && number <= 2'147'438'647) {
                type = &self->types->items[I32TypeID];
            } else {
                type = &self->types->items[I64TypeID];
            }

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
    if (!type_coercible(self->inferred_type, type)) {
        TODO("literal coercion error handling");
    }
}

void infer_ident_type(TypeInf *self, const String *ident) {
    Symbol *symbol = symbol_find(self->symbols, ident);
    if (symbol == nullptr) {
        TODO("unknown symbol error handling");
    }

    handle_known_type(self, &symbol->type);
}

void infer_compound_ident_type(TypeInf *self, const Strings *idents) {
    TODO("infer_compound_ident_type");
}

void infer_call_type(TypeInf *self, const AstCall *call) {
    Symbol *symbol = symbol_find(self->symbols, &call->name);
    if (symbol == nullptr) {
        TODO("unknown symbol error handling");
    }

    handle_known_type(self, &symbol->type);
}

void handle_known_type(TypeInf *self, const Type *type) {
    if (self->inferred_type == nullptr) {
        self->coercible = false;
        self->inferred_type = type;

        return;
    }

    if (!type_match(self->inferred_type, type) ||
            (self->coercible && !type_coercible(self->inferred_type, type))) {
        TODO("type mismatch handling");
        return;
    }

    self->coercible = false;
    self->inferred_type = type;
}
