#ifndef CITYSCREENUI_H
#define CITYSCREENUI_H

#include <vector>

#include "City.h"

class Unit;

void drawCityScreen(int centerlatitude, int centerlongitude, City *city);
void clickOnCityScreen(int lat, int lon, int lat2, int lon2);

// Food Storage box layout: itemsPerRow/colsepar picked so that up to
// getPopulationThresshold(pop) food icons (the max City::resources[0] can reach before the
// city grows) fit in the box, without a pixel-level screen capture to verify it.
void getFoodStorageLayout(int pop, int &itemsPerRow, int &colsepar);

// Units currently standing on city's tile, in a stable order shared by drawCityScreen (to
// list them) and clickOnCityScreen (to map a clicked row back to the same unit).
std::vector<Unit*> getUnitsAtCity(City* city);

#endif   // CITYSCREENUI_H