#!/bin/sh
# Line coverage of the ported code over a set of key scripts: cov.sh script...
D=$(cd "$(dirname "$0")" && pwd); P=$D/../../port
cd "$P" && cc -O0 -w -fprofile-instr-generate -fcoverage-mapping -o emu_cov ../re/emu/emu.c $(grep '^PORT_SRC' Makefile | cut -d= -f2) || exit 1
rm -rf "$D/cov"; mkdir -p "$D/cov"
i=0
for s in "$@"; do i=$((i+1)); rm -rf "$D/cov/run"; cp -R "$D/../../original" "$D/cov/run"
  LLVM_PROFILE_FILE="$D/cov/p$i.profraw" ./emu_cov -d "$D/cov/run" -o "$D/cov" -s "$D/$s" -n 8000000000 "$D/cov/run/Conflict.exe" 2>/dev/null; rm -f "$D"/cov/*.ppm "$D"/cov/*.mem; done
xcrun llvm-profdata merge -o "$D/cov/all.profdata" "$D"/cov/*.profraw
xcrun llvm-cov report ./emu_cov -instr-profile="$D/cov/all.profdata" $(ls *.c | grep -v hooks.c) 2>/dev/null | awk 'NF>9{printf "%-14s lines %5s missed %5s  %s\n", $1, $8, $9, $10}'
xcrun llvm-cov show ./emu_cov -instr-profile="$D/cov/all.profdata" $(ls *.c | grep -v hooks.c) > "$D/cov/lines.txt" 2>/dev/null
