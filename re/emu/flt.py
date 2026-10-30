import struct,subprocess,random,sys
def h(x): return '%016x'%struct.unpack('<Q',struct.pack('<d',x))[0]
def d(s): return struct.unpack('<d',struct.pack('<Q',int(s,16)))[0]
r=random.Random(5); cases=[]
for i in range(3000):
    op=r.choice(['mul','div','add','sub'])
    k=r.choice([0,1,2])
    if k==0: a=float(r.randint(-2000,2000)); b=r.choice([100.0,0.4,1.2000000000000002,1.5,1.75,3.0,7.0,0.3,17.0,float(r.randint(1,500))])
    elif k==1: a=r.randint(-30000,30000)/r.choice([100.0,3.0,7.0,10.0]); b=float(r.randint(-300,300)) or 1.0
    else: a=r.uniform(-1e4,1e4); b=r.uniform(-1e3,1e3)
    cases.append((op,a,b))
inp='\n'.join('%s %s %s'%(op,h(a),h(b)) for op,a,b in cases)+'\n'
import os
env=dict(os.environ,FLTTEST='1')
out=subprocess.run(['../../port/emu_h','run/Conflict.exe'],input=inp,capture_output=True,text=True,env=env).stdout.split('\n')
bad=0
for (op,a,b),o in zip(cases,out):
    exp={'mul':a*b,'div':(a/b if b else 0),'add':a+b,'sub':a-b}[op]
    if o!=h(exp):
        bad+=1
        if bad<=12: print(op,a,b,'emu',o,d(o) if len(o)==16 else o,'ieee',h(exp),exp)
print('mismatches',bad,'of',len(cases))
