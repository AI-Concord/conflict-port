#!/bin/sh
# Cross-builds the Windows version with MinGW-w64 (brew install mingw-w64).
set -e
cd "$(dirname "$0")"
OUT=../build/win; DIST="$OUT/Conflict"
rm -rf "$OUT"; mkdir -p "$OUT/obj" "$DIST/data"
CC=x86_64-w64-mingw32-gcc; CXX=x86_64-w64-mingw32-g++
GAME_SRC=$(grep '^GAME_SRC' ../port/Makefile | cut -d= -f2)
for f in $GAME_SRC platform.c; do $CC -O2 -w -c ../port/$f -o "$OUT/obj/${f%.c}.o"; done
$CC -O2 -w -c engine_win.c -o "$OUT/obj/engine_win.o"
x86_64-w64-mingw32-windres res/conflict.rc -O coff -o "$OUT/obj/res.o" --include-dir res
$CXX -O2 -Wall -DWIDL_EXPLICIT_AGGREGATE_RETURNS -c main.cpp -o "$OUT/obj/main.o"
$CXX -static -mwindows -o "$DIST/Conflict.exe" "$OUT"/obj/*.o -ld3d12 -ldxgi -ld3dcompiler -ldxguid -lwinmm -lshell32 -luser32 -lgdi32 -ladvapi32
x86_64-w64-mingw32-strip "$DIST/Conflict.exe"
cp ../original/*.DAT ../original/*.FNT ../original/*.MP ../port/conflict.dat "$DIST/data/"
echo "built $DIST/Conflict.exe"
