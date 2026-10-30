#!/bin/sh
# usage: cmp.sh script [maxinstr]  -> runs pure original and hybrid, compares every frame
D=$(cd "$(dirname "$0")" && pwd); P=$D/../../port
run() { # $1=binary $2=outdir $3=extra
  rm -rf "$2"; mkdir -p "$2"; rm -rf "$2.run"; cp -R "$D/../../original" "$2.run"
  "$1" $3 -d "$2.run" -o "$2" -s "$SCRIPT" ${MAXI:+-n $MAXI} "$2.run/Conflict.exe" 2>"$2/log.txt"
}
SCRIPT=$1; MAXI=$2
run "$P/emu_h" "$D/cmp_orig" -H
HOOKSTATS=1 run "$P/emu_h" "$D/cmp_port" ""
bad=0; n=0
for f in "$D"/cmp_orig/*.ppm; do b=$(basename "$f"); n=$((n+1))
  if ! cmp -s "$f" "$D/cmp_port/$b"; then bad=$((bad+1)); echo "DIFF frame $b"; fi; done
for f in "$D"/cmp_orig/*.mem; do [ -f "$f" ] || continue; b=$(basename "$f"); python3 -I "$D/memdiff.py" "$f" "$D/cmp_port/$b" || bad=$((bad+1)); done
echo "$n frames compared, $bad differ"; tail -1 "$D/cmp_orig/log.txt"; tail -1 "$D/cmp_port/log.txt"
