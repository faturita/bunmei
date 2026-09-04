#ifndef COMMERCEUI_H
#define COMMERCEUI_H

#include <vector>

class City;

// The commerce screen (controller.view == 4): opened by engine.cpp:engageTrade() when a
// human Transport steps onto a foreign city's tile at PEACE or better. A stripped city
// screen -- same footprint/background, but only two boxes, both drawn with the very code
// the city screen uses: the bottom-left box is the city's stocked resources
// (cityscreenui.cpp:drawResourceStorageBox, 1 icon / 10 units) with a cursor/right.png
// "buy" arrow per row, and the bottom-centre "Port" box is the trading Transport drawn as a
// Units-box row (cityscreenui.cpp:drawUnitsBoxRow) -- press a box.png cargo slot to sell
// that stack. Buy/sell pay through prices[] and move COINS between the faction's
// capital-city treasury and city->coreresources[COINS].
void drawCommerceScreen(int centerlatitude, int centerlongitude, City *city);
void clickOnCommerceScreen(int lat, int lon, int lat2, int lon2);

// Resource ids currently aboard the trading Transport (controller.tradeunitid), in
// getCargo() order.
std::vector<int> getTradeCargo();

// Called from map.cpp's draw path, like openCityScreen(): renders the commerce screen when
// controller.view == 4.
void openCommerceScreen();

#endif   // COMMERCEUI_H
