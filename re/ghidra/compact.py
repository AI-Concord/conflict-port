#!/usr/bin/env python3
# Compact view of decomp.c: drops local declarations and blank lines. usage: compact.py [start_fn [end_fn]]
import re,sys
import os
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')
d=open(os.path.join(ROOT,'re/ghidra/out/decomp.c')).read()
parts=re.split(r'\n//// FUNCTION ',d)[1:]
out=[]
decl=re.compile(r'^\s+(undefined\d?|short|int|uint|char|byte|bool|double|float|long|ulong|ushort|code|qword|dword|word|u?int\d*|undefined)\s*\**\s*\w+(\s*\[\d+\])?;$')
for p in parts:
    lines=p.split('\n'); head=lines[0]
    name=head.split(' ')[0]
    body=[]
    for l in lines[1:]:
        if not l.strip(): continue
        if decl.match(l): continue
        if 'unaff_DS' in l and l.strip().endswith(';') and '=' not in l: continue
        body.append(l)
    out.append((name,'//// '+head+'\n'+'\n'.join(body)))
names=[n for n,_ in out]
s=sys.argv[1] if len(sys.argv)>1 else names[0]; e=sys.argv[2] if len(sys.argv)>2 else None
i=names.index(s); j=names.index(e) if e else len(names)
for n,t in out[i:j]: print(t); print()
