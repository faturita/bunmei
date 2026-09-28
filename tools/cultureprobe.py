#!/usr/bin/env python3
"""Measure the culture model of README.md's Culture section on small synthetic maps.

temperature[t][f] = SUM over cities c of faction f of Qc / (K * (d(c,t) + 1)^P)

The spec (P = 2, the square law) was chosen from numbers this script produced, comparing it with
P = 1 and with Manhattan distance instead of the unit-movement path.  Keep it to re-run those
comparisons when tuning CULTURE_K / CULTURE_THRESHOLD, or before changing the model.

d is the cheapest path the way units move (8 neighbours, cost of ENTERING the tile, LAND only,
terrain/road/railroad costs as in tiles.h), compared against Manhattan |dlat|+|dlon|.  The +1 keeps
the city's own tile at Qc/K instead of infinite.  It prints five experiments: one city (side vs
diagonal, area), terrain (mountain ridge, lake, island across a strait), a road and a railroad,
two factions' borders (A with Q, B with 2Q), and many cities (how far the tails add up).

Usage:
  python3 tools/cultureprobe.py [P] [QSCALE]

  P       distance exponent (default 1; the spec uses 2)
  QSCALE  multiplies every city's Q (default 1).  1/d^2 at QSCALE 10 reaches as far as 1/d at 1.
"""
import heapq, sys
N=61; K=1.0; TH=1.0                      # threshold: a tile counts when some faction has T > TH
P=float(sys.argv[1]) if len(sys.argv)>1 else 1.0
QSCALE=float(sys.argv[2]) if len(sys.argv)>2 else 1.0
GRASS,MOUNT,WATER,ROAD,RAIL=1.0,3.0,None,1/3,1/9

def blank(): return [[GRASS]*N for _ in range(N)]

def path_dist(cost, src):
    D=[[float('inf')]*N for _ in range(N)]; D[src[0]][src[1]]=0; pq=[(0,src)]
    while pq:
        d,(i,j)=heapq.heappop(pq)
        if d>D[i][j]: continue
        for a in (-1,0,1):
            for b in (-1,0,1):
                if a==b==0: continue
                y,x=i+a,j+b
                if not(0<=y<N and 0<=x<N) or cost[y][x] is None: continue
                nd=d+cost[y][x]
                if nd<D[y][x]: D[y][x]=nd; heapq.heappush(pq,(nd,(y,x)))
    return D

def manhattan(src): return [[abs(i-src[0])+abs(j-src[1]) for j in range(N)] for i in range(N)]

def temp(cities, dist):   # cities: list of (Q, D-grid)
    return [[sum(Q/(K*(D[i][j]+1)**P) for Q,D in cities if D[i][j]!=float('inf')) for j in range(N)] for i in range(N)]

c=N//2; C=(c,c); Q=10.0*QSCALE

print("== 1. one city, open grassland: side vs diagonal at 7 tiles")
T_p=temp([(Q,path_dist(blank(),C))],None); T_m=temp([(Q,manhattan(C))],None)
for name,T in (("unit path",T_p),("manhattan",T_m)):
    area=sum(1 for i in range(N) for j in range(N) if T[i][j]>TH)
    print(f"  {name:9s}: side(7,0)={T[c-7][c]:.3f}  diag(7,7)={T[c-7][c+7]:.3f}  tiles above threshold={area}")

print("== 2. terrain: mountain ridge 3 tiles east, lake 3 tiles west, island 5 tiles south across water")
t=blank()
for i in range(N): t[i][c+3]=MOUNT
for i in range(c-4,c+5):
    for j in range(c-6,c-2): t[i][j]=WATER
for i in range(c+2,c+5):
    for j in range(N): t[i][j]=WATER                 # a 3-tile sea strait south of the city
T_p=temp([(Q,path_dist(t,C))],None)
for name,(i,j) in {"beyond ridge (0,+5)":(c,c+5),"behind lake (0,-7)":(c,c-7),"island (+6,0)":(c+6,c)}.items():
    print(f"  {name:22s} unit path={T_p[i][j]:.3f}  manhattan={T_m[i][j]:.3f}")

print("== 3. a road and a railroad running east from the city (10 tiles)")
for label,tc in (("road",ROAD),("railroad",RAIL)):
    t=blank()
    for j in range(c+1,c+11): t[c][j]=tc
    T=temp([(Q,path_dist(t,C))],None)
    print(f"  {label:8s}: on it at 10 tiles={T[c][c+10]:.3f}  next to it (+1,10)={T[c+1][c+10]:.3f}  plain grass at 10={T_m[c][c+10] and temp([(Q,path_dist(blank(),C))],None)[c][c+10]:.3f}")

print("== 4. two factions, A (Q=10) and B (Q=20), 12 tiles apart: who owns what (allegiance > 0.5)")
A=(c,c-6); B=(c,c+6)
def owners(dA,dB):
    TA=temp([(10.0*QSCALE,dA)],None); TB=temp([(20.0*QSCALE,dB)],None); own={}
    for i in range(N):
        for j in range(N):
            a,b=TA[i][j],TB[i][j]
            own[(i,j)]=None if max(a,b)<=TH else ('A' if a/(a+b)>0.5 else 'B' if b/(a+b)>0.5 else None)
    return own
op=owners(path_dist(blank(),A),path_dist(blank(),B)); om=owners(manhattan(A),manhattan(B))
for name,o in (("unit path",op),("manhattan",om)):
    print(f"  {name:9s}: A owns {sum(v=='A' for v in o.values())}, B owns {sum(v=='B' for v in o.values())}")
diff=sum(op[t]!=om[t] for t in op)
print(f"  tiles whose owner differs between the two: {diff}")

print("== 5. many cities: sum grows with the number of cities (1/d has a long tail)")
for n in (1,4,16):
    cities=[(Q,manhattan((c+di,c+dj))) for di in range(-12,13,24//max(1,int(n**0.5)) if n>1 else 25) for dj in range(-12,13,24//max(1,int(n**0.5)) if n>1 else 25)][:n] if n>1 else [(Q,manhattan(C))]
    T=temp(cities,None)
    print(f"  {len(cities):2d} cities: tiles above threshold={sum(1 for i in range(N) for j in range(N) if T[i][j]>TH)}  far corner (0,0)={T[0][0]:.3f}")
