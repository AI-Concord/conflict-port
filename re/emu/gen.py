import sys,random
seed=int(sys.argv[1]); n=int(sys.argv[2]); dump_from=int(sys.argv[3]) if len(sys.argv)>3 else 10**9
r=random.Random(seed+1000)
keys=['enter']*45+['down']*30+['up']*8+['end']*8+['home']*3+['right']*2+['left']*2+['space']*1+['B']*2+['S']*1
out=['key enter']*8
wars=[(7,0),(7,3),(7,4),(7,6),(2,1),(6,2),(5,0),(6,3),(4,3)]
for i in range(n):
    out.append('key '+r.choice(keys))
    if i%4==3: out.append('shot f%04d'%i)
    if i%20==19 or i>=dump_from: out.append('dump d%04d'%i)
    if r.random()<0.01:
        a,b=r.choice(wars); out.append('poke %x 1'%(0xa05e+a*0x10+b*2)); out.append('# poke at %d'%i)
    if r.random()<0.004: out.append('poke %x a'%(0xa0fe+r.randrange(8)*2)); out.append('# nukepoke at %d'%i)
print('\n'.join(out))
