#!/usr/bin/env python3
# Accurate disassembly of a code range: nd.py seg:start end [cols]   (ghidra segment numbering)
import sys,subprocess
import os
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
seg,start=[int(x,16) for x in sys.argv[1].split(':')]; end=int(sys.argv[2],16); cols=int(sys.argv[3]) if len(sys.argv)>3 else 3
out=subprocess.run(['ndisasm','-b16','-o',hex(start),'-e',str(seg*16-0x10000+start),os.path.join(ROOT,'re/image.bin')],capture_output=True,text=True).stdout
rows=[]
for l in out.split('\n'):
    p=l.split(None,2)
    if len(p)<3: continue
    try: a=int(p[0],16)
    except: continue
    if a>=end: break
    rows.append('%04X %s'%(a,p[2]))
w=max(len(r) for r in rows)+2
for i in range(0,len(rows),cols): print(''.join(r.ljust(w) for r in rows[i:i+cols]).rstrip())
