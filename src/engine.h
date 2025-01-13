#ifndef ENGINE_H
#define ENGINE_H

#include "Faction.h"
#include "City.h"

int getNextCityId();
int getNextUnitId();
int nextUnitId(int faction);
int nextMovableUnitId(int f_id);

City* findCityAt(int lat, int lon);
Unit* getDefender(int lat, int lon, int &numberofdefenders, int f_id);

#endif // ENGINE_H