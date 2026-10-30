#!/usr/bin/env python3
# Show a range of the precise listing: sn.py 1000 start end [cols]
import sys
import os
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
seg=sys.argv[1]; a=int(sys.argv[2],16); b=int(sys.argv[3],16); cols=int(sys.argv[4]) if len(sys.argv)>4 else 4
rows=[l.rstrip('\n') for l in open(os.path.join(ROOT,'re/nd_%s.txt')%seg) if a<=int(l[:4],16)<=b]
rows=[r.replace('call word 0x1007:word 0x686','I2D').replace('call word 0x1007:word 0x4d6','DMUL').replace('call word 0x1007:word 0x1b9','DADD').replace('call word 0x1007:word 0x167','DSUB').replace('call word 0x1007:word 0x390','DDIV').replace('call word 0x1007:word 0x747','D2I').replace('call word 0x1007:word 0x32f','DCMP').replace('call word ','CALL ').replace(':word ',':') for r in rows]
w=max(len(r) for r in rows)+2
for i in range(0,len(rows),cols): print(''.join(r.ljust(w) for r in rows[i:i+cols]).rstrip())
