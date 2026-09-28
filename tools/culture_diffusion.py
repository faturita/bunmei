#!/usr/bin/env python3
"""Culture model REJECTED 2026-09-28: plain heat diffusion over the 8 neighbours, equal weights.

Kept as the evidence. One city re-heats its tile every year; the tile 7 along a row and the tile
7 along a diagonal both first get heat in year 7, but diffusion adds up every path, and there is
1 path to the diagonal against 393 to the side: the side is 274x hotter at arrival, still ~5x
at steady state (the heat settles into circles, by the central limit theorem).  That is why
ring distance, not diffusion, went into the spec.

Usage:
  python3 tools/culture_diffusion.py
"""
# Equal-weight 8-neighbour diffusion from one city that re-heats its tile every year (pure python).
N=41; c=N//2; D=0.5; k=0.05; S=10.0
T=[[0.0]*N for _ in range(N)]
NB=[(a,b) for a in (-1,0,1) for b in (-1,0,1) if (a,b)!=(0,0)]
def step(T):
    T=[row[:] for row in T]; T[c][c]+=S
    U=[[0.0]*N for _ in range(N)]
    for i in range(N):
        for j in range(N):
            s=sum(T[(i+a)%N][(j+b)%N] for a,b in NB)
            U[i][j]=(T[i][j]+D*(s-8*T[i][j])/8)*(1-k)
    return U
first={}
for y in range(1,121):
    T=step(T)
    for name,(dy,dx) in {'side':(7,0),'diag':(7,7)}.items():
        if name not in first and T[c+dy][c+dx]>0: first[name]=y
    if y in (8,9,10,15,30,120):
        s,d=T[c+7][c],T[c+7][c+7]
        print(f"year {y:3d}: side(7,0)={s:.3e}  diag(7,7)={d:.3e}  side/diag={s/d:.1f}")
print("first year with any heat:", first)
