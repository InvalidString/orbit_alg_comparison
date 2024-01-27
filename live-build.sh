#!/bin/sh
set -xe

inc="-I$HOME/src/raylib/src"
lib="-L$HOME/src/raylib/src"

clang -O3 -o live.so live.c -fPIC -shared "$lib" "$inc" -lraylib -g
