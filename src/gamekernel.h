#ifndef GAMEKERNEL_H
#define GAMEKERNEL_H

void initMap();
void initFactions();
void initResources();
int getNextCityId();
int getNextUnitId();
int nextUnitId(int faction);

#endif // GAMEKERNEL_H