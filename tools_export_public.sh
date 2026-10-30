#!/bin/sh
# Copies this project's own work, without any of the original game (no game files, no data segment, no
# decompilation or disassembly listings of its code, no strings or images extracted from it, no icon made from its
# art), into ~/github-export/conflict-port, the folder that is published. Run, then commit and push there.
set -e
SRC=$(cd "$(dirname "$0")" && pwd); DST=$HOME/github-export/conflict-port
mkdir -p "$DST"
find "$DST" -mindepth 1 -maxdepth 1 ! -name .git -exec rm -rf {} +
cd "$SRC"
git ls-files | grep -v -E '^(original/|re/ghidra/out/|app/Resources/|win/res/(conflict\.ico|icon\.png))' \
  | grep -v -E '^(port/conflict\.dat|re/image\.bin|re/strings\.txt|re/gfx_nd\.txt|re/nd_[0-9a-f]+\.txt|re/emu/crash\.txt)$' \
  | while IFS= read -r f; do mkdir -p "$DST/$(dirname "$f")"; cp -p "$f" "$DST/$f"; done
echo "exported $(cd "$DST" && find . -type f ! -path './.git/*' | wc -l | tr -d ' ') files to $DST"
