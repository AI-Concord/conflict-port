#!/usr/bin/env python3
# Dump code bytes: cs.py seg:off [len]
import sys
import os
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
d=open(os.path.join(ROOT,'original/Conflict.exe'),'rb').read()
seg,off=[int(x,16) for x in sys.argv[1].split(':')]; n=int(sys.argv[2],0) if len(sys.argv)>2 else 32
a=7744+(seg*16-0x10000)+off; b=d[a:a+n]
for i in range(0,len(b),16): print('%04x: '%(off+i)+' '.join('%02x'%x for x in b[i:i+16]))
