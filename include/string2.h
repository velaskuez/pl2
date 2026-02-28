#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "array2.h"
#include "util.h"

const char *read_file(int fd) {
    off_t len = lseek(fd, 0, SEEK_END);
    if (len == -1) {
        panic("lseek: %s", strerror(errno));
    }

    if (lseek(fd, 0, SEEK_SET) == -1) {
        panic("lseek: %s", strerror(errno));
    }

    char *contents = malloc(len);

    ssize_t n = read(fd, contents, len);
    if (n != len) {
        panic("read less than expected bytes: %ld", n);
    }

    return contents;
}

int string2_cmp(const char *s, const char *t) {
    if (len(s) != len(t)) return -1;
    return strcmp(s, t);
}

void string2_append_cstr(char *s, const char *t) {
    for (const char *c = t; *c; c++) {
        append(s, *c);
    }
}

void string2_append_string(char *s, const char *t) {
    foreach(c, t) {
        append(s, *c);
    }
}
