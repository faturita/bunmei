#!/usr/bin/env python3
"""Follow-up to culture_tree.py: the same tree with the city order shuffled every year.

Kept as the evidence that order was not the problem. The cities become equal on average (8.28 vs
8.60) but jump 5.7..12.1 year to year, and the middle stays drained at 5.07 against 19.92; heat
is pushed to the outside (far sides 7.9 vs 7.1).

Usage:
  python3 tools/culture_tree_random.py
"""
# Same whiteboard tree rule as culture_tree_probe.py, but the order in which cities make their
# pass is shuffled every year.  Settled values: mean and min..max over the last 50 of 200 years.
import random
N=41; R=15; a=0.5; k=0.1; S=10.0
def sgn(v): return (v>0)-(v<0)
def children(dy,dx):
    d=max(abs(dy),abs(dx))
    if d==0: return [(y,x) for y in (-1,0,1) for x in (-1,0,1) if (y,x)!=(0,0)]
    sy=sgn(dy) if abs(dy)==d else 0; sx=sgn(dx) if abs(dx)==d else 0
    if sy and sx: return [(dy+sy,dx),(dy+sy,dx+sx),(dy,dx+sx)]
    if sy: return [(dy+sy,dx)]
    return [(dy,dx+sx)]
def year(T, cities):
    for (cy,cx) in cities: T[cy][cx]+=S
    for (cy,cx) in cities:
        for d in range(R-1,-1,-1):
            moves=[]
            for dy in range(-d,d+1):
                for dx in range(-d,d+1):
                    if max(abs(dy),abs(dx))!=d: continue
                    t=((cy+dy)%N,(cx+dx)%N); h=a*T[t[0]][t[1]]
                    if h: moves.append((t,h,[((cy+y)%N,(cx+x)%N) for y,x in children(dy,dx)]))
            for t,h,ch in moves:
                T[t[0]][t[1]]-=h
                for q in ch: T[q[0]][q[1]]+=h
    return [[v*(1-k) for v in row] for row in T]
random.seed(1)
c=N//2; A=(c,c-3); B=(c,c+3)
probes={'city A':A,'city B':B,'middle':(c,c),'far side A':(c,c-9),'far side B':(c,c+9),'5 above middle':(c-5,c)}
def stats(cities, shuffle, years=200, tail=50):
    T=[[0.0]*N for _ in range(N)]; rec={p:[] for p in probes}
    for y in range(years):
        order=cities[:]
        if shuffle: random.shuffle(order)
        T=year(T,order)
        if y>=years-tail:
            for p,(i,j) in probes.items(): rec[p].append(T[i][j])
    return {p:(sum(v)/len(v),min(v),max(v)) for p,v in rec.items()}
rnd=stats([A,B],True); TA=stats([A],False); TB=stats([B],False)
print(f"{'tile':16s} random order: mean (min .. max)      A alone + B alone")
for p in probes:
    m,lo,hi=rnd[p]
    print(f"{p:16s} {m:8.3f} ({lo:7.3f} .. {hi:7.3f})      {TA[p][0]+TB[p][0]:10.3f}")
