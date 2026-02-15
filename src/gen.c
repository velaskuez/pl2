#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "gen.h"
#include "symbol.h"
#include "array.h"
#include "ast.h"
#include "str.h"
#include "type.h"
#include "util.h"

static void init_scoped_symbols(Generator *self);
static void free_scoped_symbols(Generator *self);

static TypeID gen_type(Generator *self, const AstType *ast_type);
static void gen_struct(Generator *self, const AstStruct *ast_struct);
static void gen_function(Generator *self, const AstFunction *function);
static void gen_block(Generator *self, const AstBlock *block);
static void gen_statements(Generator *self, const AstStatements *statements);
static void gen_statement(Generator *self, const AstStatement *statement);
static void gen_assign(Generator *self, const AstAssign *assign);
static void gen_let(Generator *self, const AstLet *let);
static void gen_return(Generator *self, const AstExpr *return_);
static void gen_if(Generator *self, const AstIf *if_);
static void gen_while(Generator *self, const AstWhile *while_);

static void gen_expr(Generator *self, const AstExpr *expr);
static void gen_binary_op(Generator *self, const AstBinaryOp *binary_op);
static void gen_unary_op(Generator *self, const AstUnaryOp *unary_op);
static void gen_value(Generator *self, const AstValue *value);
static void gen_ident(Generator *self, const String *ident);
static void gen_compound_ident(Generator *self, const Strings *idents);
static void gen_call(Generator *self, const AstCall *call);

void gen_init(Generator *self) {
    init_scoped_symbols(self);

    Type types[] = PRIMITIVE_TYPES;

    for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); i++) {
        append(&self->types, types[i]);
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
        // TODO: if we decide to generate IR -> stack (or other backend), then we
        // can move this check later
        if (typeid > I8PtrTypeID && !ast_field->type.pointer) {
            TODO("structs must only contain pointers to other structs");
        }

        // We need to resolve all of the field types before we can calculate
        // the offsets of them
        StructField field = {0};
        field.id = typeid;
        // field.offset = later
        append(&struct_.field_types, field);
    }

    size_t offset = 0;
    size_t padding = 0;
    for (size_t i = 0; i < struct_.field_types.len; i++) {
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

void gen_function(Generator *self, const AstFunction *function) {
    // Save the tmp count to reset at the end - this will be useful
    // when we come to implement scoped functions
    int tmpvar = self->tmpvar;

    TypeID return_typeid = gen_type(self, function->return_type);
    if (return_typeid < 0) TODO("handle unknown type");

    TypeIDs arg_typeids = {0};
    foreach(param, &function->params) {
        TypeID typeid = gen_type(self, &param->type);
        if (typeid < 0) TODO("handle unknown type");

        append(&arg_typeids, typeid);
        append(&self->symbols->head, symbol_make_variable(param->name, typeid, self->tmpvar++));
    }

    append(&self->symbols->head, symbol_make_function(function->name, return_typeid, arg_typeids));

    gen_block(self, &function->block);

    self->tmpvar = tmpvar; // Reset
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
        gen_expr(self, &statement->as.expr);
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
