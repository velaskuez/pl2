#!/bin/bash

set -ex

ensure_dir() {
    if [[ ! -e "$1" ]]; then
        echo "Setting up $1" >&2
        mkdir -p "$1"
    fi
}

ensure_dir "build/debug";
ensure_dir "build/test";

build_object() {
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
        -c $1 \
        -o $2
}

for file in $(ls src); do
    build_object "src/$file" "build/debug/${file%.*}.o"
done

if [[ $1 == "test" ]]; then
    build_object "thirdparty/Unity/unity.c" "build/test/unity.o"

    # use the same objs, but remove main to avoid duplicating symbol
    cp build/debug/*.o build/test/
    rm build/test/main.o

    for file in $(ls tests); do
        obj="build/test/${file%.*}.o"
        bin="build/test/${file%.*}"

        build_object "tests/$file" $obj

        cc build/test/*.o -o $bin
        ./$bin
        rm $obj # so next test doesn't duplicate main
    done
else
    cc build/debug/*.o -o build/debug/main
    ./build/debug/main
fi
