#ifndef AI_H
#define AI_H

#include "units/Unit.h"

coordinate goTo(Unit* unit, bool &ok);
coordinate goTo(Unit* unit, bool &ok, int targetlat, int targetlon);
coordinate reachableHorizon(Unit* unit, int jumpingdistance, int f_id, bool &ok);
int determineLandMass(coordinate c);
bool isGoodCitySpot(int lat, int lon);
coordinate findCitySpot(coordinate from, bool &found);

void autoPlayer();
void autoPlayerMoveUnits();
void autoPlayerCities();

#endif // AI_H