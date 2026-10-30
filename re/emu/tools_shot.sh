#!/bin/sh
# usage: tools_shot.sh script outdir  -> runs emu, converts ppm to png
D=$(dirname "$0"); rm -rf "$2"; mkdir -p "$2"; rm -rf "$D/run"; cp -R "$D/../../original" "$D/run"
"$D/../../port/emu_h" -H -d "$D/run" -o "$2" -s "$1" ${3:+-n $3} "$D/run/Conflict.exe" 2>"$2/log.txt"
for f in "$2"/*.ppm; do sips -s format png "$f" --out "${f%.ppm}.png" >/dev/null 2>&1; rm "$f"; done
tail -3 "$2/log.txt"
