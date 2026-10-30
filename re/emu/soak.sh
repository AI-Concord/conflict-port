#!/bin/sh
# Long random sessions on the stand-alone port; reports hangs (timeout) or crashes.
D=$(cd "$(dirname "$0")" && pwd); P=$D/../../port
for seed in "$@"; do
  python3 -I - "$seed" > "$D/soak_$seed.txt" <<'PY'
import sys,random
r=random.Random(int(sys.argv[1])+1000)
keys=['enter']*45+['down']*30+['up']*8+['end']*8+['home']*3+['right']*2+['left']*2+['space']*1+['B']*2+['S']*1
out=['key enter']*8
wars=[(7,0),(7,3),(7,4),(7,6),(2,1),(6,2),(5,0),(6,3),(4,3)]
for i in range(4000):
    out.append('key '+r.choice(keys))
    if r.random()<0.004:
        a,b=r.choice(wars); out.append('poke %x 1'%(0xa05e+a*0x10+b*2))
    if r.random()<0.002: out.append('poke %x a'%(0xa0fe+r.randrange(8)*2))
print('\n'.join(out))
PY
  rm -rf "$D/soak.run" "$D/soak.out"; cp -R "$D/../../original" "$D/soak.run"; mkdir -p "$D/soak.out"
  perl -e 'alarm 60; exec @ARGV' "$P/conflict_headless" -D "$P/conflict.dat" -d "$D/soak.run" -o "$D/soak.out" -s "$D/soak_$seed.txt" 2>"$D/soak.out/log.txt"
  echo "seed $seed: exit status $? ($(tail -1 "$D/soak.out/log.txt"))"
done
