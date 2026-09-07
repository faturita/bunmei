#ifndef AUTOMATION_H
#define AUTOMATION_H

#include <vector>

#include "units/Unit.h"

coordinate goTo(Unit* unit, bool &ok);
coordinate goTo(Unit* unit, bool &ok, int targetlat, int targetlon);
coordinate reachableHorizon(Unit* unit, int jumpingdistance, int f_id, bool &ok);
int determineLandMass(coordinate c);
bool isGoodCitySpot(int lat, int lon);
coordinate findCitySpot(coordinate from, int faction, bool &found);

// `count` distinct random LAND tiles for faction start positions (no two the same).
// Fewer than `count` only if the map has fewer land tiles than that.
std::vector<coordinate> pickFactionStartTiles(int count);

void autoPlayer();
void autoPlayerMoveUnits();
void autoPlayerCities();

void processGoTo();

#endif // AUTOMATION_H