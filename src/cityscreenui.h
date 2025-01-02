#ifndef CITYSCREENUI_H
#define CITYSCREENUI_H

#include "City.h"

void drawCityScreen(int centerlatitude, int centerlongitude, City *city);
void clickOnCityScreen(int lat, int lon, int lat2, int lon2);

#endif   // CITYSCREENUI_H