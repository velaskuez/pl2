#!/bin/bash

cc \
    -I./include \
    -I./thirdparty/Unity \
    -Wall \
    -Wextra \
    -pedantic \
    -std=c23 \
    -g \
    -Wno-char-subscripts \
    -Wno-gnu-statement-expression-from-macro-expansion \
    -Wno-gnu-case-range \
    src/* \
    -o build/main

./build/main
