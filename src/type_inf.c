#include <stdbool.h>
#include <assert.h>

#include "str.h"
#include "type.h"
#include "type_inf.h"
#include "ast.h"
#include "symbol.h"
#include "util.h"
#include "array.h"
#include "report.h"

static void type_expr_inner(TypeInf *self, const AstExpr *expr);
static void type_expr_binary_op(TypeInf *self, const AstBinaryOp *binary_op);
static void type_expr_unary_op(TypeInf *self, const AstUnaryOp *unary_op);
static void type_expr_value(TypeInf *self, const AstValue *value);
static void type_expr_ident(TypeInf *self, const String *ident);
static void type_expr_compound_ident(TypeInf *self, const Strings *idents);
static void type_expr_call(TypeInf *self, const AstCall *call);

static Type *type_location_ident(TypeInf *self, const String *ident);
static Type *type_location_compound_ident(TypeInf *self, const Strings *idents);
static Type *type_location_index(TypeInf *self, const AstIndex *index);

static void handle_known_type(TypeInf *self, const Type *type);

// TODO: type_infer can either return a pointer to a type in the types array or a symbol type
// which is messy. This makes me think that the typeid approach is probably more consistent
// even though it requires a few extra lines of code whereever it's used. It's also safer in case
// of reallocations and requires less space than storing the Type struct or possibly
// even a pointer to it.
TypeInf type_infer_new(Report *report, const Types *types,
                       const Structs *structs, const SymbolChain *symbols) {
    TypeInf self = {0};
    self.coercible = false;
    self.types = types;
    self.structs = structs;
    self.symbols = symbols;
    self.report = report;

    return self;
}

Inferred type_expr(TypeInf *self, const AstExpr *expr) {
    type_expr_inner(self, expr);

    Inferred inferred_type = {0};
    inferred_type.type = self->inferred_type;
    inferred_type.is_coercible = self->coercible;

    return inferred_type;
}

Type* type_location(TypeInf *self, const AstLocation *location) {
    switch (location->kind) {
    case LocationIdent:
        return type_location_ident(self, &location->as.ident);
    case LocationCompoundIdent:
        return type_location_compound_ident(self, &location->as.compound_ident);
    case LocationIndex:
        return type_location_index(self, &location->as.index);
    }
}

bool type_match(const Type *t, const Type *u) {
    if (string_cmp(&t->key, &u->key) != 0) return false;
    if (t->pointer != u->pointer) return false;

    return true;
}

bool type_coercible(const Type *t, const Type *u) {
    // Can T be coerced into U?
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

void type_expr_inner(TypeInf *self, const AstExpr *expr) {
    switch (expr->kind) {
    case ExprBinaryOp:
        type_expr_binary_op(self, &expr->as.binary_op);
        break;
    case ExprUnaryOp:
        type_expr_unary_op(self, &expr->as.unary_op);
        break;
    case ExprValue:
        type_expr_value(self, &expr->as.value);
        break;
    case ExprIdent:
        type_expr_ident(self, &expr->as.ident);
        break;
    case ExprCompoundIdent:
        type_expr_compound_ident(self, &expr->as.compound_ident);
        break;
    case ExprCall:
        type_expr_call(self, &expr->as.call);
        break;
    }
}

void type_expr_binary_op(TypeInf *self, const AstBinaryOp *binary_op) {
    switch (binary_op->op) {
    case BinaryOpIndex:
        // rhs - must be a 64bit number
        TypeInf self0 = *self;
        self0.inferred_type = nullptr;
        self0.coercible = false;
        type_expr_inner(&self0, binary_op->right);
        if (self0.inferred_type == nullptr) {
            report_error(self->report, "could not infer type of index expression");
            return;
        }

        if (!type_match(self0.inferred_type, &self->types->items[I64TypeID]) &&
            !(self0.coercible && type_coercible(self0.inferred_type, &self->types->items[I64TypeID]))) {
            report_error(self->report, "index expression must be i64");
            return;
        }

        // lhs - dereferenced type becomes the inferred type.
        type_expr_inner(self, binary_op->left);
        if (self->inferred_type == nullptr) {
            report_error(self->report, "could not infer type of indexed value");
            return;
        }

        // TODO(type-refactor): support multiple dereferences.
        // Currently this condition is never reached, eg
        // a is *i8;
        // a[0][0] will always resolve to i8, which is wrong.
        if (!self->inferred_type->pointer) {
            report_error(self->report, "value cannot be indexed");
            return;
        }

        Type *dereferenced_type = nullptr;
        foreach(type, self->types) {
            if (string_cmp(&type->key, &self->inferred_type->key) != 0) continue;
            if (!type->pointer) {
                dereferenced_type = type;
                break;
            }
        }

        if (dereferenced_type == nullptr) {
            report_internal_error(self->report, "non-pointer type should exist in type table");
            return;
        }

        self->inferred_type = dereferenced_type;

        break;
    default:
        type_expr_inner(self, binary_op->left);
        type_expr_inner(self, binary_op->right);
        break;
    }
}

void type_expr_unary_op(TypeInf *self, const AstUnaryOp *unary_op) {
    Type *type = nullptr;

    switch (unary_op->op) {
    case UnaryOpNew:
        if (unary_op->expr->kind != ExprIdent) {
            report_error(self->report, "argument to new must be an identifier");
            return;
        }

        foreach(it, self->types) {
            if (string_cmp(&it->key, &unary_op->expr->as.ident) != 0) continue;
            if (!it->pointer) continue;

            type = it;
            break;
        }

        if (type == nullptr) {
            report_error(self->report, "unknown type *%.*s", STRING_FMT_ARGS(&unary_op->expr->as.ident));
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

void type_expr_value(TypeInf *self, const AstValue *value) {
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
        report_error(self->report, "literal can not be coerced to %s%.*s",
                     self->inferred_type->pointer ? "*" : "",
                     STRING_FMT_ARGS(&self->inferred_type->key));
        return;
    }
}

void type_expr_ident(TypeInf *self, const String *ident) {
    Symbol *symbol = symbol_find(self->symbols, ident);
    if (symbol == nullptr) {
        report_error(self->report, "unknown identifier %.*s", STRING_FMT_ARGS(ident));
        return;
    }

    handle_known_type(self, &symbol->type);
}

void type_expr_compound_ident(TypeInf *self, const Strings *idents) {
    TODO("type_expr_compound_ident");
}

void type_expr_call(TypeInf *self, const AstCall *call) {
    Symbol *symbol = symbol_find(self->symbols, &call->name);
    if (symbol == nullptr) {
        report_error(self->report, "unknown identifier %.*s", STRING_FMT_ARGS(&call->name));
        return;
    }

    handle_known_type(self, &symbol->type);
}

Type *type_location_ident(TypeInf *self, const String *ident) {
    Symbol *symbol = symbol_find(self->symbols, ident);
    if (symbol == nullptr) {
        report_error(self->report, "unknown identifier %.*s", STRING_FMT_ARGS(ident));
        return nullptr;
    }

    if (symbol->kind != SymbolVariable) {
        report_error(self->report, "only assignment of variables currently supported");
        return nullptr;
    }

    return &symbol->type;
}

Type *type_location_compound_ident(TypeInf *self, const Strings *idents) {
    assert(idents->len > 1);

    Symbol *symbol = symbol_find(self->symbols, &idents->items[0]);
    if (symbol == nullptr) {
        report_error(self->report, "unknown identifier %.*s", STRING_FMT_ARGS(&idents->items[0]));
        return nullptr;
    }

    Type *resolved_type = &symbol->type;
    if (symbol->kind != SymbolVariable) {
        report_error(self->report, "cannot access field of %.*s", STRING_FMT_ARGS(&resolved_type->key));
        return nullptr;
    }

    assert(resolved_type->struct_id > 0);

    Struct *struct_ = &self->structs->items[resolved_type->struct_id];

    for (const String *field_name = &idents->items[1]; field_name < idents->items + idents->len; field_name++) {
        if (resolved_type->struct_id < 0) {
            report_error(self->report, "cannot access field of %.*s", STRING_FMT_ARGS(&resolved_type->key));
            return nullptr;
        }

        StructField *struct_field = nullptr;
        foreach(it, &struct_->fields) {
            if (string_cmp(&it->key, field_name) != 0) continue;
            struct_field = it;
        }

        if (struct_field == nullptr) {
            report_error(self->report, "%.*s is not a field in %.*s", STRING_FMT_ARGS(field_name), STRING_FMT_ARGS(&resolved_type->key));
            return nullptr;
        }

        resolved_type = &self->types->items[struct_field->id];
        if (resolved_type->struct_id >= 0) {
            struct_ = &self->structs->items[resolved_type->struct_id];
        }
    }

    return resolved_type;
}

Type *type_location_index(TypeInf *self, const AstIndex *index) {
    Symbol *symbol = symbol_find(self->symbols, &index->ident);

    if (symbol->kind != SymbolVariable) {
        report_error(self->report, "only assignment of variables currently supported");
        return nullptr;
    }

    if (!symbol->type.pointer) {
        report_error(self->report, "cannot index into %.*s", STRING_FMT_ARGS(&index->ident));
        return nullptr;
    }


    // Find the non-pointer type
    // TODO(type-refactor): as part of the type refactor it would be worth
    // attempting to support multiple dereferences, and maybe have
    // some nice helper like dereference_type(type)
    foreach(type, self->types) {
        if (string_cmp(&type->key, &symbol->type.key) != 0) continue;
        if (!type->pointer) return type;
    }

    report_internal_error(self->report, "expected non-pointer type to exist in type table");

    return nullptr;
}

void handle_known_type(TypeInf *self, const Type *type) {
    if (self->inferred_type == nullptr) {
        self->coercible = false;
        self->inferred_type = type;

        return;
    }

    if (!type_match(self->inferred_type, type) ||
            (self->coercible && !type_coercible(self->inferred_type, type))) {
        report_type_mismatch_error(self->report, type, self->inferred_type);
        return;
    }

    self->coercible = false;
    self->inferred_type = type;
}
