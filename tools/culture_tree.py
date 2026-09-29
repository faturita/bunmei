#!/usr/bin/env python3
"""Culture model REJECTED 2026-09-28: faturita's whiteboard tree over a SHARED temperature.

Kept as the evidence. The tree (side tile -> 1 child straight out, corner -> 3) gives exact rings
for ONE city (every ring-7 tile 4.4626).  Run for two cities in turn over one shared field, each
pass also moves the other city's heat: the first city processed ends with 13.30 on its tile, the
second 5.41, and the tile between them gets 5.32 instead of the 19.92 the two would add up to.

Usage:
  python3 tools/culture_tree.py
"""
# faturita's whiteboard rule: every tile of ring d (around a centre) passes heat to its children in ring
# d+1 -- a side tile to 1 tile straight out, a corner tile to 3 (straight, diagonal, straight).  Each
# child receives a*T[parent] and the parent loses a*T once.  One pass per city per year over the SHARED
# temperature field, outermost ring first so heat moves one ring per year; then global Newton cooling.
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
    for (cy,cx) in cities:
        T[cy][cx]+=S
    for (cy,cx) in cities:
        for d in range(R-1,-1,-1):
            ring=[(dy,dx) for dy in range(-d,d+1) for dx in range(-d,d+1) if max(abs(dy),abs(dx))==d]
            moves=[]
            for dy,dx in ring:
                t=((cy+dy)%N,(cx+dx)%N); h=a*T[t[0]][t[1]]
                if h==0: continue
                moves.append((t,h,[((cy+y)%N,(cx+x)%N) for y,x in children(dy,dx)]))
            for t,h,ch in moves:
                T[t[0]][t[1]]-=h
                for q in ch: T[q[0]][q[1]]+=h
    return [[v*(1-k) for v in row] for row in T]
def run(cities, years=150):
    T=[[0.0]*N for _ in range(N)]
    for _ in range(years): T=year(T,cities)
    return T
c=N//2
T=run([(c,c)])
ring=[T[c-7][c+x] for x in range(-7,8)]+[T[c+y][c+7] for y in range(-7,8)]
print("single city, ring 7: min %.4f max %.4f  (side %.4f, corner %.4f)"%(min(ring),max(ring),T[c-7][c],T[c-7][c-7]))
A=(c,c-3); B=(c,c+3)
for order in ([A,B],[B,A]):
    T=run(order); tag="A,B" if order[0]==A else "B,A"
    print(f"two cities order {tag}: middle {T[c][c]:8.3f}  far-A {T[c][c-9]:7.3f}  far-B {T[c][c+9]:7.3f}  above-middle {T[c-5][c]:7.3f}  cityA {T[A[0]][A[1]]:7.2f} cityB {T[B[0]][B[1]]:7.2f}")
TA=run([A]); TB=run([B])
print(f"A alone + B alone:          middle {TA[c][c]+TB[c][c]:8.3f}  far-A {TA[c][c-9]+TB[c][c-9]:7.3f}  far-B {TA[c][c+9]+TB[c][c+9]:7.3f}  above-middle {TA[c-5][c]+TB[c-5][c]:7.3f}")
