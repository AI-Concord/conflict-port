import sys,random
seed=int(sys.argv[1]); n=int(sys.argv[2])
r=random.Random(seed+5000)
keys=['enter']*45+['down']*30+['up']*8+['end']*8+['home']*3+['right']*2+['left']*2+['0x3042']*3+['0x1f53']*2
out=['key enter']*8
def W(a,v): return 'poke %x %x'%(a,v&0xffff)
def rc(): return r.randrange(7)
pokes=[
 lambda: W(0xa05e+7*0x10+r.choice([0,3,4,6])*2,1),                 # Israel at war with a neighbour
 lambda: W(0xa05e+r.choice([(2,1),(6,2),(5,0),(6,3),(4,3)])[0]*0x10+0,1),
 lambda: W(0xb2d6+rc()*2,r.choice([1,2])),                         # covert order
 lambda: W(0xa15e+r.choice([0,3,4,6])*2,r.choice([1,2])),          # peace talks
 lambda: W(0xb274,r.choice([5,6,11,12])),                          # month
 lambda: W(0xb262,r.choice([100,140,160])),                        # tension
 lambda: W(0xa0fe+r.randrange(8)*2,r.choice([7,8,9,10])),          # nuclear stage
 lambda: W(0x9898,r.choice([3,6])),                                # special aircraft in reserve
 lambda: W(0x980c+r.choice([0,3,4,6])*0x14,r.choice([2,5])),       # special aircraft at a front
 lambda: W(0x98be,r.choice([0,2,6])),                              # brigade pool
 lambda: W(0x939e,r.choice([40,300,1500,5000])),                   # treasury
 lambda: W(0xb264,r.choice([0,12,35,60])),                         # condemnation
 lambda: W(0xa10e+rc()*2,r.choice([5,8,9])),                       # government stability
 lambda: W(0xa11e+rc()*2,r.choice([0,4,9])),                       # unrest
 lambda: W(0xa05e+7*0x10+rc()*2,r.choice([2,3,5,7,9,10])),         # relations with Israel
 lambda: W(0x9380+r.choice([0,3,4,6])*2,r.choice([1,2,4])),        # months of war with Israel
 lambda: W(0x98a0+r.choice([0,3,4,6])*2,r.choice([-3,0,3,5])),     # front line
 lambda: W(0xa12c,r.choice([2,6,9])),                              # unrest in Israel
 lambda: W(0xb26a,r.choice([0,1])), lambda: W(0x98c0,r.choice([0,1])), lambda: W(0xb266,r.choice([3,6])),
]
for i in range(n):
    out.append('key '+r.choice(keys))
    if i%4==3: out.append('shot f%04d'%i)
    if i%20==19: out.append('dump d%04d'%i)
    if i>12 and r.random()<0.05: out.append(r.choice(pokes)())
print('\n'.join(out))
