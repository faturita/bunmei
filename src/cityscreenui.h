#ifndef CITYSCREENUI_H
#define CITYSCREENUI_H

#include <vector>

#include "City.h"

class Unit;

void drawCityScreen(int centerlatitude, int centerlongitude, City *city);
void clickOnCityScreen(int lat, int lon, int lat2, int lon2);

// Tiles a rectangular border (top/bottom/left/right/corner .png set) in map space, 16px
// tiles, from (clo+startleft, cla+starttop) to (clo+endright, cla+endbottom). Adjacent
// boxes share a border line (no gap). Also used by the commerce screen (commerceui.cpp).
void drawBoundingBox(int clo, int cla, int startleft, int starttop, int endright, int endbottom);

// The bottom-left "Resource Storage" box: label + border at (clo,cla,-10,4,-4,9), then one
// scrollable row per stocked resource -- an icon strip of 1 icon / 10 units (floored, capped
// at 30), the exact count, and `arrowAsset` pinned at column -5 -- plus up/down scroll
// arrows once the list overflows. `scrollOffset` is the caller's own offset variable (0 =
// top), clamped here. Shared by the city screen ("load" arrow) and the commerce screen
// ("buy" arrow); the caller's click handler maps the arrow / scroll clicks.
void drawResourceStorageBox(int cla, int clo, City* city, int& scrollOffset,
                            const char* label, const char* arrowAsset);

// One "Units" box row (drawn at lat cla+(5+loc)): the unit's faction-tinted icon, status
// overlays, name, and -- for a Transport -- one assets/assets/city/box.png cargo slot per
// capacity() slot (a loaded slot shows the resource icon under the frame). Shared by the
// city screen's Units box and the commerce screen's "port" box. Click handling (slot s at
// fine-grid lon2 == s-4) lives in each screen's own click handler.
void drawUnitsBoxRow(int cla, int clo, class Unit* u, int loc);

// Food Storage box layout: itemsPerRow/colsepar picked so that getPopulationThresshold(pop)
// food icons (the max City::resources[0] can reach before the city grows) fill EVERY row
// the box has (itemsPerRow is the tightest fit across all rows) and colsepar -- a float,
// meant to be applied per-icon with round(), not truncated once for the whole row -- spaces
// them to reach exactly the box's right edge, using its full width too.
void getFoodStorageLayout(int pop, int &itemsPerRow, float &colsepar);

// Units currently standing on city's tile, in a stable order shared by drawCityScreen (to
// list them) and clickOnCityScreen (to map a clicked row back to the same unit).
std::vector<Unit*> getUnitsAtCity(City* city);

// Resources actually in storage (city->commodities[id]>0 then city->mfggoods[id]>0), in a
// stable order -- stocked commodities first (ALL_COMMODITIES order), then stocked mfg goods
// (ALL_MFG_GOODS order) -- shared by drawCityScreen (the "Resource Storage" box) and
// clickOnCityScreen (its "load" arrow).
std::vector<int> getStockedResources(City* city);

void initCoreResources();

#endif   // CITYSCREENUI_H