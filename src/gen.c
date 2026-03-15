#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "gen.h"
#include "symbol.h"
#include "array.h"
#include "ast.h"
#include "str.h"
#include "type.h"
#include "type_inf.h"
#include "util.h"
#include "stack.h"
#include "report.h"

static void init_scoped_symbols(Generator *self);
static void free_scoped_symbols(Generator *self);
static int next_var(Generator *self, const Type *type);
static Inferred get_inferred_type(Generator *self, const AstExpr *expr);
static Type *get_location_type_ident(Generator *self, const String *ident);
static Type *get_location_type_compound_ident(Generator *self, const Strings *idents);
static Type *get_location_type_index(Generator *self, const AstIndex *index);
static Type *get_location_type(Generator *self, const AstLocation *location);
static void report_type_mismatch(Generator *self, const Type *want, const Type *have);
static int printf_with_newline(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

static TypeID gen_type(Generator *self, const AstType *ast_type);
static void gen_struct(Generator *self, const AstStruct *ast_struct);
static void gen_function(Generator *self, const AstFunction *function);
static void gen_block(Generator *self, const AstBlock *block);
static void gen_statements(Generator *self, const AstStatements *statements);
static void gen_statement(Generator *self, const AstStatement *statement);
static void gen_location(Generator *self, const AstLocation *location);
static void gen_location_ident(Generator *self, const String *ident);
static void gen_location_compound_ident(Generator *self, const Strings *idents);
static void gen_location_index(Generator *self, const AstIndex *index);
static void gen_assign(Generator *self, const AstAssign *assign);
static void gen_let(Generator *self, const AstLet *let);
static void gen_statement_expr(Generator *self, const AstExpr *return_);
static void gen_return(Generator *self, const AstExpr *return_);
static void gen_if(Generator *self, const AstIf *if_);
static void gen_while(Generator *self, const AstWhile *while_);

static void gen_expr(Generator *self, const AstExpr *expr, const Type *type);
static void gen_binary_op(Generator *self, const AstBinaryOp *binary_op, const Type *type);
static void gen_unary_op_new(Generator *self, const AstExpr *expr, const Type *type);
static void gen_unary_op(Generator *self, const AstUnaryOp *unary_op, const Type *type);
static void gen_value(Generator *self, const AstValue *value, const Type * type);
static void gen_ident(Generator *self, const String *ident, const Type *type);
static void gen_compound_ident(Generator *self, const Strings *idents, const Type *type);
static void gen_call(Generator *self, const AstCall *call, const Type *type);

void gen_init(Generator *self) {
    init_scoped_symbols(self);

    Type types[] = PRIMITIVE_TYPES;

    for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); i++) {
        append(&self->types, types[i]);
    }

    self->write_fn = printf_with_newline;
}

void gen_file(Generator *self, const AstFile *file) {
    self->write_fn(".entry main\n");

    foreach(struct_, &file->structs) {
        gen_struct(self, struct_);
    }

    foreach(function, &file->functions) {
        gen_function(self, function);
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
    // It shouldn't be possible for the pointer type to
    // be defined before the actual type.
    assert(ast_type->pointer);
    Type type = self->types.items[foundid];
    type.pointer = true;
    type.size = 8;
    type.slotsize = DoubleSlot;
    type.alignment = 8;
    type.struct_id = self->types.items[foundid].struct_id;
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
        // TODO: if we decide to generate IR -> stack (or other backend), then we
        // can move this check later
        if (self->types.items[typeid].struct_id > 0 && !ast_field->type.pointer) {
            TODO("structs must only contain pointers to other structs");
        }

        // We need to resolve all of the field types before we can calculate
        // the offsets of them
        StructField field = {0};
        field.key = ast_field->name;
        field.id = typeid;
        // field.offset = later
        append(&struct_.fields, field);
    }

    size_t offset = 0;
    size_t padding = 0;
    for (size_t i = 0; i < struct_.fields.len; i++) {
        StructField *it = &struct_.fields.items[i];

        it->offset = offset;
        offset += self->types.items[it->id].size;

        // If padding < 8, then we may have room to add in the next field
        padding = 8 - (offset % 8);
        if (padding == 0 || padding == 8) {
            continue;
        }

        if (i == struct_.fields.len-1) {
            // Last element - add padding so the size of the struct
            // will be a mulitple of 8
            offset += padding;
            break;
        }

        // Two fields can be directly adjacent if either
        //  - They both have the same alignment
        //  - The first field is placed so that the next field will
        //    be correctly aligned
        size_t field_alignment = self->types.items[it->id].alignment;
        size_t next_field_alignment = self->types.items[struct_.fields.items[i+1].id].alignment;
        if (field_alignment == next_field_alignment ||
                (field_alignment % offset == 0 && next_field_alignment < field_alignment)) {
            continue;
        }

        offset += padding;
    }

    // At this point, offset == size
    Type type = {0};
    type.key = ast_struct->name;
    type.realsize = type.size = offset;
    type.pointer = false;
    type.alignment = 8;
    type.slotsize = Invalid;
    type.struct_id = self->structs.len;
    append(&self->types, type);

    struct_.id = self->types.len-1;
    append(&self->structs, struct_);

    return;
}

void gen_function(Generator *self, const AstFunction *function) {
    // Save the tmp count to reset at the end - this will be useful
    // when we come to implement scoped functions
    int var = self->var;

    TypeID return_typeid = gen_type(self, function->return_type);
    if (return_typeid < 0) TODO("handle unknown type");

    Type return_type = self->types.items[return_typeid];

    Types arg_types = {0};
    foreach(param, &function->params) {
        TypeID typeid = gen_type(self, &param->type);
        if (typeid < 0) TODO("handle unknown type");

        Type type = self->types.items[typeid];
        append(&arg_types, type);
        append(&self->symbols->head, symbol_make_variable(param->name, type, next_var(self, &type)));
    }

    append(&self->symbols->head, symbol_make_function(function->name, return_type, arg_types));

    self->write_fn("%.*s:", STRING_FMT_ARGS(&function->name));

    gen_block(self, &function->block);

    self->var = var; // Reset
}

void gen_block(Generator *self, const AstBlock *block) {
    init_scoped_symbols(self);
    gen_statements(self, &block->statements);
    free_scoped_symbols(self);
}

void gen_statements(Generator *self, const AstStatements *statements) {
    foreach(statement, statements) {
        gen_statement(self, statement);
    }
}

void gen_statement(Generator *self, const AstStatement *statement) {
    switch (statement->kind) {
    case StatementAssign:
        gen_assign(self, &statement->as.assign);
        break;
    case StatementLet:
        gen_let(self, &statement->as.let);
        break;
    case StatementExpr:
        gen_statement_expr(self, &statement->as.expr);
        break;
    case StatementReturn:
        gen_return(self, statement->as.return_);
        break;
    case StatementIf:
        gen_if(self, &statement->as.if_);
        break;
    case StatementWhile:
        gen_while(self, &statement->as.while_);
        break;
    }
}

void gen_location(Generator *self, const AstLocation *location) {
    switch (location->kind) {
    case LocationIdent:
        gen_location_ident(self, &location->as.ident);
        break;
    case LocationCompoundIdent:
        gen_location_compound_ident(self, &location->as.compound_ident);
        break;
    case LocationIndex:
        gen_location_index(self, &location->as.index);
        break;
    }
}

void gen_location_ident(Generator *self, const String *ident) {
    Symbol *symbol = symbol_find(self->symbols, ident);

    // We should have done these checks earlier whilst type-checking the assignment value
    assert(symbol != nullptr);
    assert(symbol->kind == SymbolVariable);

    self->write_fn("store%s %d", op_ext(&symbol->type), symbol->as.local);
}

void gen_location_compound_ident(Generator *self, const Strings *idents) {
    Symbol *symbol = symbol_find(self->symbols, &idents->items[0]);

    // We should have done these checks earlier whilst type-checking the assignment value
    assert(symbol != nullptr);
    assert(symbol->kind == SymbolVariable);

    Type *resolved_type = &symbol->type;

    assert(resolved_type->struct_id > 0);

    Struct *struct_ = &self->structs.items[resolved_type->struct_id];

    int var = self->var;
    int local = symbol->as.local;
    int offset = 0;

    for (const String *field_name = &idents->items[1]; field_name < idents->items + idents->len; field_name++) {
        if (resolved_type->struct_id < 0) {
            report_error(&self->report, "cannot access field of %.*s", STRING_FMT_ARGS(&resolved_type->key));
            return;
        }

        StructField *struct_field = nullptr;
        foreach(it, &struct_->fields) {
            if (string_cmp(&it->key, field_name) != 0) continue;
            struct_field = it;
        }

        if (struct_field == nullptr) {
            report_error(&self->report, "%.*s is not a field in %.*s", STRING_FMT_ARGS(field_name), STRING_FMT_ARGS(&resolved_type->key));
            return;
        }

        offset = struct_field->offset;
        resolved_type = &self->types.items[struct_field->id];

        // Invalid access will be caught out in the next iteration, if there is one
        if (resolved_type->struct_id < 0) continue;

        struct_ = &self->structs.items[resolved_type->struct_id];

        // TODO: Clearly possible to support embedded structs, but this condition is asserted
        // earlier. Will need to track offsets in the embedded case
        assert(resolved_type->pointer);

        // TODO: should be possible to avoid doing a load immediately after a store
        self->write_fn("load.d %d", local);
        self->write_fn("push.d %d", offset);
        self->write_fn("aload.d");

        local = next_var(self, resolved_type);
        self->write_fn("store.d %d", local);
    }

    self->write_fn("load.d %d", local);
    self->write_fn("push.d %d", offset);
    self->write_fn("astore%s", op_ext(resolved_type));

    self->var = var;

    return;
}

void gen_location_index(Generator *self, const AstIndex *index) {
    Symbol *symbol = symbol_find(self->symbols, &index->ident);

    // We should have done these checks earlier whilst type-checking the assignment value
    assert(symbol != nullptr);
    assert(symbol->kind == SymbolVariable);

    Type type = symbol->type;
    type.pointer = false;

    Type *index_type = &self->types.items[I64TypeID];
    Inferred inferred = get_inferred_type(self, &index->expr);
    if (inferred.type == nullptr) {
        report_error(&self->report, "cannot infer type of expression");
        return;
    }

    if (!type_match_or_coercible(&inferred, index_type)) {
        report_type_mismatch(self, index_type, inferred.type);
        return;
    }

    self->write_fn("load.d %d", symbol->as.local);
    gen_expr(self, &index->expr, index_type);
    self->write_fn("mul.d %d", type.realsize);
    self->write_fn("astore%s", op_ext(&type));

}

void gen_assign(Generator *self, const AstAssign *assign) {
    Inferred inferred = get_inferred_type(self, &assign->expr);
    if (inferred.type == nullptr) {
        report_error(&self->report, "cannot infer type of expression");
        return;
    }

    Type *type = get_location_type(self, &assign->location);
    if (type == nullptr) {
        report_error(&self->report, "cannot resolve location type");
        return;
    }

    if (!type_match_or_coercible(&inferred, type)) {
        report_type_mismatch(self, type, inferred.type);
        return;
    }

    gen_expr(self, &assign->expr, type);
    gen_location(self, &assign->location);

    return;
}

void gen_let(Generator *self, const AstLet *let) {
    if (symbol_find(self->symbols, &let->name) != nullptr) {
        report_error(&self->report, "redefinition of %.*s", STRING_FMT_ARGS(&let->name));
        return;
    }

    // TODO: tidy up
    if (let->expr == nullptr && let->type == nullptr) {
        report_error(&self->report, "cannot declare variable without a type");
        return;
    }

    if (let->expr == nullptr) {
        TypeID typeid = gen_type(self, let->type);
        if (typeid < 0) {
            report_error(&self->report, "undefined type: %s%.*s",
                         let->type->pointer ? "*" : "",
                         STRING_FMT_ARGS(&let->type->name));
            return;
        }

        Type *type = &self->types.items[typeid];

        int var = next_var(self, type);
        append(&self->symbols->head, symbol_make_variable(let->name, *type, var));

        return;
    }

    Inferred inferred = get_inferred_type(self, let->expr);
    if (inferred.type == nullptr) {
        report_error(&self->report, "cannot infer type of expression");
        return;
    }

    if (let->type != nullptr) {
        TypeID typeid = gen_type(self, let->type);
        if (typeid < 0) {
            report_error(&self->report, "undefined type: %s%.*s",
                         let->type->pointer ? "*" : "",
                         STRING_FMT_ARGS(&let->type->name));
            return;
        }

        Type *type = &self->types.items[typeid];
        if (!type_match_or_coercible(&inferred, type)) {
            report_type_mismatch(self, type, inferred.type);
            return;
        }

        inferred.type = type;
    }

    int var = next_var(self, inferred.type);
    append(&self->symbols->head, symbol_make_variable(let->name, *inferred.type, var));

    if (let->expr == nullptr) return;

    gen_expr(self, let->expr, inferred.type);

    self->write_fn("store%s %d", op_ext(inferred.type), var);
}

void gen_statement_expr(Generator *self, const AstExpr *expr) {
    Inferred inferred = get_inferred_type(self, expr);
    if (inferred.type == nullptr) {
        report_error(&self->report, "cannot infer type of expression");
        return;
    }

    gen_expr(self, expr, inferred.type);
}

void gen_return(Generator *self, const AstExpr *return_) {
    // TODO: will need to access function context here to check
    // return types

    if (return_ == nullptr) {
        self->write_fn("ret");
        return;
    }

    Inferred inferred = get_inferred_type(self, return_);
    if (inferred.type == nullptr) {
        report_error(&self->report, "could not infer type of expression");
        return;
    }

    gen_expr(self, return_, inferred.type);
    self->write_fn("ret.%s", ret_ext(inferred.type));

    return;
}

void gen_if(Generator *self, const AstIf *if_) {
    return;
}

void gen_while(Generator *self, const AstWhile *while_) {
    return;
}

void gen_expr(Generator *self, const AstExpr *expr, const Type *type) {
    switch (expr->kind) {
    case ExprBinaryOp:
        gen_binary_op(self, &expr->as.binary_op, type);
        break;
    case ExprUnaryOp:
        gen_unary_op(self, &expr->as.unary_op, type);
        break;
    case ExprValue:
        gen_value(self, &expr->as.value, type);
        break;
    case ExprIdent:
        gen_ident(self, &expr->as.ident, type);
        break;
    case ExprCompoundIdent:
        gen_compound_ident(self, &expr->as.compound_ident, type);
        break;
    case ExprCall:
        gen_call(self, &expr->as.call, type);
        break;
    }
}

void gen_binary_op(Generator *self, const AstBinaryOp *binary_op, const Type *type) {
    if (binary_op->op == BinaryOpIndex) {
        gen_expr(self, binary_op->left, type);

        // This should have been checked earlier - the rhs must be I64
        gen_expr(self, binary_op->right, &self->types.items[I64TypeID]);
        self->write_fn("mul.d %d", type->realsize);
        self->write_fn("aload%s", op_ext(type));
        return;
    }

    gen_expr(self, binary_op->left, type);
    gen_expr(self, binary_op->right, type);

    switch (binary_op->op){
    case BinaryOpEq:
        break;
    case BinaryOpNeq:
        break;
    case BinaryOpLt:
        break;
    case BinaryOpLe:
        break;
    case BinaryOpGt:
        break;
    case BinaryOpGe:
        break;
    case BinaryOpAdd:
        self->write_fn("add%s", op_ext(type));
        break;
    case BinaryOpSub:
        self->write_fn("sub%s", op_ext(type));
        break;
    case BinaryOpMul:
        self->write_fn("mul%s", op_ext(type));
        break;
    case BinaryOpDiv:
        self->write_fn("div%s", op_ext(type));
        break;
    case BinaryOpAnd:
        break;
    case BinaryOpOr:
        break;
    case BinaryOpBitAnd:
        panic("unimplemented");
        break;
    case BinaryOpBitOr:
        panic("unimplemented");
        break;
    case BinaryOpIndex:
        panic("unreachable");
        break;
    }
}

void gen_unary_op_sizeof(Generator *self, const AstExpr *expr) {
    Inferred inferred = {0};

    switch (expr->kind) {
        case ExprIdent:
            // The identifer could be a type
            const String *ident = &expr->as.ident;
            foreach(it, &self->types) {
                if (string_cmp(&it->key, ident) == 0) {
                    inferred.type = it;
                }
            }

            if (inferred.type != nullptr) break;

            // fallthrough
        default:
            inferred = get_inferred_type(self, expr);
            if (inferred.type == nullptr) {
                report_error(&self->report, "cannot infer type of expression");
            }
            break;
    }

    self->write_fn("push.d %d", inferred.type->realsize);
}

void gen_unary_op_new(Generator *self, const AstExpr *expr, const Type *type) {
    // We should have done these checks earlier whilst type-checking the assignment value
    assert(expr->kind == ExprIdent);

    self->write_fn("push.d %d", type->realsize);
    self->write_fn("alloc");
}

void gen_unary_op(Generator *self, const AstUnaryOp *unary_op, const Type *type) {
    switch (unary_op->op) {
        case UnaryOpSizeOf:
            gen_unary_op_sizeof(self, unary_op->expr);
            break;
        case UnaryOpNew:
            gen_unary_op_new(self, unary_op->expr, type);
            break;
    }
}

void gen_value(Generator *self, const AstValue *value, const Type *type) {
    switch (value->kind) {
    case ValueString:
        // TODO: keep track of seen string literals for reuse
        int label = self->string++;
        self->write_fn(".data s%d .string \"%.*s\"", label, STRING_FMT_ARGS(&value->as.string));
        self->write_fn("dataptr s%d", label);

        return;
    case ValueChar:
        self->write_fn("push%s '%c'", op_ext(type), (char)value->as.char_);

        return;
    case ValueNumber:
        self->write_fn("push%s %ld", op_ext(type), value->as.number);

        return;
    }
}

void gen_ident(Generator *self, const String *ident, const Type *type) {
    Symbol *symbol = symbol_find(self->symbols, ident);
    if (symbol == nullptr) {
        report_error(&self->report, "unknown identifier %.*s", STRING_FMT_ARGS(ident));
        return;
    }

    if (symbol->kind != SymbolVariable) {
        report_error(&self->report, "functions not supported as values in expressions");
        return;
    }

    self->write_fn("load%s %d", op_ext(type), symbol->as.local);
}

void gen_compound_ident(Generator *self, const Strings *idents, const Type *type) {
    return;
}

void gen_call(Generator *self, const AstCall *call, const Type *type) {
    return;
}

void init_scoped_symbols(Generator *self) {
    SymbolChain symbols = {0};
    SymbolChain *next = self->symbols;
    self->symbols = box(symbols);
    self->symbols->next = next;
}

void free_scoped_symbols(Generator *self) {
    SymbolChain *symbols = self->symbols->next;
    free(self->symbols);
    self->symbols = symbols;
}

int next_var(Generator *self, const Type *type) {
    // TODO: for stack, we need to bump this up by two for types that
    // take up two slots. But it would be better to decouple this stage
    // of the compilation from stack. Perhaps store this along with the type
    // in the IR, then bump the local during IR -> stack.
    int var = self->var;

    switch (type->slotsize) {
    case SingleSlot:
        self->var++;
        break;
    case DoubleSlot:
        self->var+=2;
        break;
    case Invalid:
        TODO("Invalid");
        break;
    }

    return var;
}

Inferred get_inferred_type(Generator *self, const AstExpr *expr) {
    return type_infer(&self->types, &self->structs, self->symbols, expr);
}

Type *get_location_type_ident(Generator *self, const String *ident) {
    Symbol *symbol = symbol_find(self->symbols, ident);
    if (symbol == nullptr) {
        report_error(&self->report, "unknown identifier %.*s", STRING_FMT_ARGS(ident));
        return nullptr;
    }

    if (symbol->kind != SymbolVariable) {
        report_error(&self->report, "only assignment of variables currently supported");
        return nullptr;
    }

    return &symbol->type;
}

Type *get_location_type_compound_ident(Generator *self, const Strings *idents) {
    assert(idents->len > 1);

    Symbol *symbol = symbol_find(self->symbols, &idents->items[0]);
    if (symbol == nullptr) {
        report_error(&self->report, "unknown identifier %.*s", STRING_FMT_ARGS(&idents->items[0]));
        return nullptr;
    }

    Type *resolved_type = &symbol->type;
    if (symbol->kind != SymbolVariable) {
        report_error(&self->report, "cannot access field of %.*s", STRING_FMT_ARGS(&resolved_type->key));
        return nullptr;
    }

    assert(resolved_type->struct_id > 0);

    Struct *struct_ = &self->structs.items[resolved_type->struct_id];

    for (const String *field_name = &idents->items[1]; field_name < idents->items + idents->len; field_name++) {
        if (resolved_type->struct_id < 0) {
            report_error(&self->report, "cannot access field of %.*s", STRING_FMT_ARGS(&resolved_type->key));
            return nullptr;
        }

        StructField *struct_field = nullptr;
        foreach(it, &struct_->fields) {
            if (string_cmp(&it->key, field_name) != 0) continue;
            struct_field = it;
        }

        if (struct_field == nullptr) {
            report_error(&self->report, "%.*s is not a field in %.*s", STRING_FMT_ARGS(field_name), STRING_FMT_ARGS(&resolved_type->key));
            return nullptr;
        }

        resolved_type = &self->types.items[struct_field->id];
        if (resolved_type->struct_id >= 0) {
            struct_ = &self->structs.items[resolved_type->struct_id];
        }
    }

    return resolved_type;
}

Type *get_location_type_index(Generator *self, const AstIndex *index) {
    Symbol *symbol = symbol_find(self->symbols, &index->ident);

    if (symbol->kind != SymbolVariable) {
        report_error(&self->report, "only assignment of variables currently supported");
        return nullptr;
    }

    if (!symbol->type.pointer) {
        report_error(&self->report, "cannot index into %.*s", STRING_FMT_ARGS(&index->ident));
        return nullptr;
    }


    // Find the non-pointer type
    // TODO(type-refactor): as part of the type refactor it would be worth
    // attempting to support multiple dereferences, and maybe have
    // some nice helper like dereference_type(type)
    foreach(type, &self->types) {
        if (string_cmp(&type->key, &symbol->type.key) != 0) continue;
        if (!type->pointer) return type;
    }

    report_internal_error(&self->report, "expected non-pointer type to exist in type table");

    return nullptr;
}

Type *get_location_type(Generator *self, const AstLocation *location) {
    switch (location->kind) {
        case LocationIdent:
            return get_location_type_ident(self, &location->as.ident);
        case LocationCompoundIdent:
            return get_location_type_compound_ident(self, &location->as.compound_ident);
        case LocationIndex:
            return get_location_type_index(self, &location->as.index);
    }
}

static void report_type_mismatch(Generator *self, const Type *want, const Type *have) {
    report_error(&self->report, "types don't match\n"
                 " ~ want %s%.*s\n"
                 " ~ have %s%.*s",
                 want->pointer ? "*" : "",
                 STRING_FMT_ARGS(&want->key),
                 have->pointer ? "*" : "",
                 STRING_FMT_ARGS(&have->key));
    return;
}

int printf_with_newline(const char* fmt, ...) {
    int n = 0;

    va_list args;
    va_start(args, fmt);
    n += vprintf(fmt, args);
    va_end(args);
    n += printf("\n");

    return n;
}
