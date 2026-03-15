typedef struct {
    String filename;
    Tokens tokens;
    size_t position;
} Parser;

AstFile parse_file(Parser *self);
