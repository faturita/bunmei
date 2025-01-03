#ifndef ENGINE_H
#define ENGINE_H

int getNextCityId();
int getNextUnitId();
int nextUnitId(int faction);
int nextMovableUnitId(int faction, int currentid=0);

#endif // ENGINE_H