#!/bin/sh
# usage: cmp2.sh script  -> original (emulated) vs stand-alone native port
D=$(cd "$(dirname "$0")" && pwd); P=$D/../../port
SCRIPT=$1
rm -rf "$D/c2_orig" "$D/c2_port" "$D/c2_orig.run" "$D/c2_port.run"; mkdir -p "$D/c2_orig" "$D/c2_port"
cp -R "$D/../../original" "$D/c2_orig.run"; cp -R "$D/../../original" "$D/c2_port.run"
"$P/emu_h" -H -d "$D/c2_orig.run" -o "$D/c2_orig" -s "$SCRIPT" -n 9000000000 "$D/c2_orig.run/Conflict.exe" 2>"$D/c2_orig/log.txt"
"$P/conflict_headless" -D "$P/conflict.dat" -d "$D/c2_port.run" -o "$D/c2_port" -s "$SCRIPT" 2>"$D/c2_port/log.txt"
bad=0; n=0; first=""
for f in "$D"/c2_orig/*.ppm; do b=$(basename "$f"); n=$((n+1))
  if ! cmp -s "$f" "$D/c2_port/$b"; then bad=$((bad+1)); [ -z "$first" ] && first=$b; fi; done
mb=0
for f in "$D"/c2_orig/*.mem; do [ -f "$f" ] || continue; b=$(basename "$f"); STANDALONE=1 python3 -I "$D/memdiff.py" "$f" "$D/c2_port/$b" || mb=$((mb+1)); done
echo "$n frames compared, $bad differ${first:+ (first $first)}; $mb memory dumps differ"
