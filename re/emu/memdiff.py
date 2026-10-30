import sys
a=open(sys.argv[1],'rb').read(); b=open(sys.argv[2],'rb').read()
DS=(0x0120+0x128e)*16
# compare initialised data + BSS up to the stack area (0xb300 is where SS begins)
lo,hi=0,0xb2b8
d=[i for i in range(lo,hi) if a[DS+i]!=b[DS+i]]
# known scratch used only by the graphics module
import os
ign=set(range(0x8318,0x83a2))|(set(range(0xb290,0xb2a0))|set(range(0x7e00,0x7ff0))|set(range(0x7ff4,0x8318))|{0xb2a6,0xb2a7,0xb2a8,0xb2a9,0xb2aa,0xb2ab,0xb2ac,0xb2ad,0xb2ae,0xb2af,0xb2b0,0xb2b1,0xb2b2,0xb2b3,0xb2b4,0xb2b5} if os.environ.get('STANDALONE') else set())|set(range(0x8100,0x8220))|set(range(0xb27e,0xb290))|{0x92ac,0x92ad,0x7f17,0x91a2,0x91a3,0x91a4,0x91a5}
d=[i for i in d if i not in ign]
if d:
    print('MEMDIFF %s: %d bytes differ, first: %s'%(sys.argv[1].split('/')[-1],len(d),' '.join('%04x'%i for i in d[:24]))); sys.exit(1)
