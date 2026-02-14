#include <stdbool.h>

#include "type.h"
#include "ast.h"

// We also need to build up a table of symbols
// Returns type_id (index into types)
size_t get_expr_type(Types *self, const AstExpr *expr) {
    return 0;
}

// An expression of integer literals may evaluate to i32 by default,
// but a let binding may specify the type as a i8 or i64, which is permitted
// Some coercions aren't permitted such as int to bool, or pointer to non-pointer
bool can_coerce_types(Types *self, size_t t, size_t u) {
    return 0;
}
