#!/usr/bin/env python3
# Dump initialised DGROUP data from the EXE. usage: ds.py <hexoff> [len] [s|w|b|ptrs]
import sys,struct
import os
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
d=open(os.path.join(ROOT,'original/Conflict.exe'),'rb').read()
HDR=7744; DS=0x228e0-0x10000
def at(off,n): return d[HDR+DS+off:HDR+DS+off+n]
off=int(sys.argv[1],16); n=int(sys.argv[2],0) if len(sys.argv)>2 else 64; mode=sys.argv[3] if len(sys.argv)>3 else 'b'
b=at(off,n)
if mode=='s':
    i=0
    while i<len(b):
        j=b.find(b'\0',i)
        if j<0: j=len(b)
        if j>i: print('%04x: %r'%(off+i,b[i:j].decode('latin1')))
        i=j+1
elif mode=='w':
    for i in range(0,len(b)-1,16):
        print('%04x: '%(off+i)+' '.join('%5d'%struct.unpack_from('<h',b,i+k)[0] for k in range(0,min(16,len(b)-i),2)))
else:
    for i in range(0,len(b),16):
        c=b[i:i+16]; print('%04x: '%(off+i)+' '.join('%02x'%x for x in c)+'  '+''.join(chr(x) if 32<=x<127 else '.' for x in c))
