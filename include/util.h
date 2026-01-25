#define box(data) ({                           \
    auto value = data;                         \
    void *allocation = malloc(sizeof(value));  \
    memcpy(allocation, &value, sizeof(value)); \
    allocation;                                \
})

#define panic(msg, ...) \
    fprintf(stderr, "%s:%d: "msg"\n", __FILE__, __LINE__, __VA_ARGS__); \
    exit(1)

#define TODO(msg) \
    panic("%s: "msg, "TODO")

