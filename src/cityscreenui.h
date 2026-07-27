#ifndef CITYSCREENUI_H
#define CITYSCREENUI_H

#include <vector>

#include "City.h"

class Unit;

void drawCityScreen(int centerlatitude, int centerlongitude, City *city);
void clickOnCityScreen(int lat, int lon, int lat2, int lon2);

// Units currently standing on city's tile, in a stable order shared by drawCityScreen (to
// list them) and clickOnCityScreen (to map a clicked row back to the same unit).
std::vector<Unit*> getUnitsAtCity(City* city);

#endif   // CITYSCREENUI_H