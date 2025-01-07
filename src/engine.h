#ifndef ENGINE_H
#define ENGINE_H

#include "City.h"

int getNextCityId();
int getNextUnitId();
int nextUnitId(int faction);
int nextMovableUnitId(int faction);

City* findCityAt(int lat, int lon);

#endif // ENGINE_H