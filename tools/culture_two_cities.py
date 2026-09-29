#!/usr/bin/env python3
"""Why a single max-based field per faction was REJECTED 2026-09-28.

Kept as the evidence. Two cities 6 tiles apart: with one max field per faction the middle tile
gets 51.2 -- exactly what ONE city gives it; one field per city, added per faction, gives 102.4.
A shared number cannot tell "another city's heat" (add it) from "the same city's heat by another
path" (do not), so reinforcement needs the source to be remembered.

Usage:
  python3 tools/culture_two_cities.py
"""
# Two cities of the SAME faction, 6 tiles apart on a row; probe the tile in the middle (3 from each).
# (1) one field per faction, max rule   (2) one field per city, max rule, then summed per faction.
N=31; k=0.1; g_=0.8; S=10.0; r=N//2
A=(r,r-3); B=(r,r+3); MID=(r,r); FAR=(r,r-9)   # FAR: 6 from A, 12 from B
NB=[(a,b) for a in (-1,0,1) for b in (-1,0,1) if (a,b)!=(0,0)]
def run(sources, years=300):
    T=[[0.0]*N for _ in range(N)]
    for _ in range(years):
        U=[[0.0]*N for _ in range(N)]
        for i in range(N):
            for j in range(N):
                best=T[i][j]*(1-k)
                for a,b in NB: best=max(best,T[(i+a)%N][(j+b)%N]*g_)
                U[i][j]=best+sum(S for s in sources if s==(i,j))
        T=U
    return T
one=run([A,B]); fa=run([A]); fb=run([B])
for name,p in (('middle (3 from each)',MID),('far side (6 from A)',FAR)):
    print(f"{name:22s} one-field-max={one[p[0]][p[1]]:7.2f}   per-city-sum={fa[p[0]][p[1]]+fb[p[0]][p[1]]:7.2f}   A alone={fa[p[0]][p[1]]:7.2f}")
