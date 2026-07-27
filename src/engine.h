#ifndef ENGINE_H
#define ENGINE_H

#include "Faction.h"
#include "City.h"
#include "units/Unit.h"

int getNextCityId();
int getNextUnitId();
int nextUnitId(int faction);
int nextMovableUnitId(int f_id);

City* findCityAt(int lat, int lon);
Unit* getDefender(int lat, int lon, int &numberofdefenders, int f_id);

int findNearbyEnemyFactionId(int unitId, int radius);

// Makes u the active/selectable unit (coordinator.a_u_id) and wakes it out of whatever
// passive state it was in: a fortified/sentried unit packs up/wakes up, a working one
// (isWorking()) has its improvement interrupted (Unit::completed(), no finalize command --
// a later order starts the effort over from scratch).  Shared by the map-view unit click
// (usercontrols.cpp) and the city screen's stationed-unit icons (cityscreenui.cpp).
void activateUnit(Unit* u);

#endif // ENGINE_H