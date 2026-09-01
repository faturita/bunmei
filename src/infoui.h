#ifndef INFOUI_H
#define INFOUI_H

#include <vector>

class City;

void drawInfoScreen();

// Cities belonging to the given faction, in cities' own iteration order. Shared by
// drawInfoScreen (to list them) and its testcase (to verify the filtering independently of
// rendering).
std::vector<City*> getCitiesForFaction(int faction_id);

#endif   // INFOUI_H
