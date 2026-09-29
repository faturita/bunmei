#!/usr/bin/env python3
"""Culture model REJECTED 2026-09-28: a cellular automaton that takes the MAX of its neighbours.

Kept as the evidence. T'[t] = max(T[t]*(1-k), max over land neighbours of T[n]*gamma) + g[t]
gives exact rings (every ring-7 tile 100*0.8^7 = 20.97) and goes around a lake (behind it:
0.8^8), but with max two cities of the same faction do not add up -- see culture_two_cities.py.

Usage:
  python3 tools/culture_ring_automaton.py
"""
# Candidate culture cellular automaton, one faction, one city at the centre.
#   T'[t] = max( T[t]*(1-k), max over LAND neighbours n of T[n]*gamma[n] ) + g[t]
# g = heat generated on the tile (city CULTURE), gamma = how much a tile passes on, k = cooling.
N=41; c=N//2; k=0.1; S=10.0
land=[[True]*N for _ in range(N)]
for i in range(c+2,c+5):            # a lake south-east of the city, to see heat go around it
    for j in range(c-1,c+6): land[i][j]=False
gamma=[[0.8]*N for _ in range(N)]
NB=[(a,b) for a in (-1,0,1) for b in (-1,0,1) if (a,b)!=(0,0)]
def step(T):
    U=[[0.0]*N for _ in range(N)]
    for i in range(N):
        for j in range(N):
            if not land[i][j]: continue
            best=T[i][j]*(1-k)
            for a,b in NB:
                n=((i+a)%N,(j+b)%N)
                if land[n[0]][n[1]]: best=max(best,T[n[0]][n[1]]*gamma[n[0]][n[1]])
            U[i][j]=best+(S if (i,j)==(c,c) else 0.0)
    return U
T=[[0.0]*N for _ in range(N)]
first={}
probes={'side(-7,0)':(-7,0),'diag(-7,-7)':(-7,-7),'diag(-7,7)':(-7,7),'ring7 other(-7,3)':(-7,3),'behind lake(6,2)':(6,2)}
for y in range(1,201):
    T=step(T)
    for name,(dy,dx) in probes.items():
        if name not in first and T[c+dy][c+dx]>0: first[name]=y
print("first year with heat:", first)
print("year 200:", {n: round(T[c+dy][c+dx],4) for n,(dy,dx) in probes.items()})
print("steady city tile S/k =", S/k, " measured", round(T[c][c],3))
