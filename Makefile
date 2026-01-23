# MAKEFLAGS += --silent

CFLAGS=-I./include -I./thirdparty/Unity -Wall -Wextra -pedantic -std=c23 -g -Wno-char-subscripts -Wno-gnu-statement-expression-from-macro-expansion -Wno-gnu-case-range

TESTS = $(wildcard tests/*.c)
SRC = $(wildcard src/*.c)
OBJS = $(SRC:src/%.c=build/%.o)
TARGET=main

TEST_BINS := $(TESTS:tests/%.c=build/%)

run: $(TARGET)
	./main

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_BINS)
	@for t in $^; do ./$$t; done

build/%: tests/%.c
	@mkdir -p build
	cc $(CFLAGS) src/string.c src/token.c thirdparty/Unity/unity.c $< -o $@

build:
	mkdir build

clean:
	rm -rf build $(TARGET)
