#ifndef GAMEKERNEL_H
#define GAMEKERNEL_H

void initMap();
void initFactions();
int getNextCityId();
int getNextUnitId();
int nextUnitId(int faction);

#endif // GAMEKERNEL_H