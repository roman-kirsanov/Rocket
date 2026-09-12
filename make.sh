#!/usr/bin/env bash

# Rocket build driver.
#
#   ./make.sh              debug build
#   ./make.sh --test       debug build, then run the test suite
#   ./make.sh --asan       debug build with AddressSanitizer
#   ./make.sh --release    optimised build, tests off
#
# Flags combine: ./make.sh --asan --example --test

set -e

DIR=$(dirname $(realpath $0))
BUILD=Debug
OUT=$DIR/.build/Debug
TESTING=ON
TESTS=0
ASAN=""
ASAN_LINK=""

if [[ $@ == *--release* ]]; then
    BUILD=Release
    OUT=$DIR/.build/Release
    TESTING=OFF
fi

if [[ $@ == *--asan* ]]; then
    ASAN="-fsanitize=address -fno-omit-frame-pointer"
    ASAN_LINK="-fsanitize=address"
fi

if [[ $@ == *--test* ]]; then
    TESTING=ON
    TESTS=1
fi

cmake -S $DIR -B $OUT \
    -D CMAKE_BUILD_TYPE=$BUILD \
    -D CMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -D CMAKE_C_FLAGS="$ASAN" \
    -D CMAKE_CXX_FLAGS="$ASAN" \
    -D CMAKE_OBJCXX_FLAGS="$ASAN" \
    -D CMAKE_EXE_LINKER_FLAGS="$ASAN_LINK" \
    -D BUILD_TESTING=$TESTING

cmake --build $OUT --parallel 8

if [[ $TESTS == 1 ]]; then
    ctest --test-dir $OUT --output-on-failure
fi
