#!/usr/bin/env python3
# Generate a random key script: fuzz.py seed nkeys > script
import sys,random
r=random.Random(int(sys.argv[1])); n=int(sys.argv[2])
keys=['enter']*45+['down']*30+['up']*8+['end']*5+['home']*3+['right']*2+['left']*2+['space']*2+['esc']*1+['B']*2
out=['key enter']*8
for i in range(n):
    out.append('key '+r.choice(keys)); out.append('shot f%04d'%i)
    if i%10==9: out.append('dump d%04d'%i)
print('\n'.join(out))
