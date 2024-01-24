#!/bin/sh

set -xe

inc="-I$HOME/src/raylib/src"
lib="-L$HOME/src/raylib/src"

gcc -o live.so live.c -fPIC -shared "$lib" "$inc" -lraylib -lm

gcc -o main main.c "$lib" "$inc" -lraylib -ldl -lm
patchelf --add-rpath "." main
