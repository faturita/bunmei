#ifndef CITYSCREENUI_H
#define CITYSCREENUI_H

#include <vector>

#include "City.h"

class Unit;

void drawCityScreen(int centerlatitude, int centerlongitude, City *city);
void clickOnCityScreen(int lat, int lon, int lat2, int lon2);

// Food Storage box layout: itemsPerRow/colsepar picked so that getPopulationThresshold(pop)
// food icons (the max City::resources[0] can reach before the city grows) fill EVERY row
// the box has (itemsPerRow is the tightest fit across all rows) and colsepar -- a float,
// meant to be applied per-icon with round(), not truncated once for the whole row -- spaces
// them to reach exactly the box's right edge, using its full width too.
void getFoodStorageLayout(int pop, int &itemsPerRow, float &colsepar);

// Units currently standing on city's tile, in a stable order shared by drawCityScreen (to
// list them) and clickOnCityScreen (to map a clicked row back to the same unit).
std::vector<Unit*> getUnitsAtCity(City* city);

// Commodities actually in storage (city->commodities[id]>0), in a stable order shared by
// drawCityScreen (the "Commodities Storage" box) and clickOnCityScreen (its "load" arrow).
std::vector<int> getStockedCommodities(City* city);

void initCoreResources();

#endif   // CITYSCREENUI_H