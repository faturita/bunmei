#ifndef GAMEKERNEL_H
#define GAMEKERNEL_H

void initMap();
void initFactions();
void initResources();
int getNextCityId();
int getNextUnitId();
int nextUnitId(int faction);
int nextMovableUnitId(int faction, int currentid=0);

#endif // GAMEKERNEL_H