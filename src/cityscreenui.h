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

// Storage icons (food, shields) are drawn one after the other this many px apart.
#define STORAGE_ICON_PITCH 8

// Food Storage box layout for getPopulationThresshold(pop) food icons (the most
// City::resources[FOOD] holds before the city grows): one after the other, STORAGE_ICON_PITCH
// apart, squeezed (colsepar, a float applied per-icon with round()) only when they would not
// fit. With a Granary the first half and the second half each start on their own row:
// granaryRow is the row the second half starts on (0 without a Granary).
void getFoodStorageLayout(int pop, bool granary, int &itemsPerRow, float &colsepar, int &granaryRow);

// Grid position (column, row) of food icon i in that layout.
void getFoodIconSlot(int i, int pop, int itemsPerRow, int granaryRow, int &col, int &row);

// Same idea as getFoodStorageLayout but for the bottom-right "Change" box where produced
// shields accumulate: the grid holds the SHIELDS the queued buildable NEEDS (requiredShields,
// from factoryRequirement()), one after the other, squeezed only when they would not fit.
void getProductionStorageLayout(int requiredShields, int &itemsPerRow, float &colsepar);

// Treasure box layout (right column, between the buildings and the Change box): one coin icon
// per coin in city->resources[COINS]. Rows are filled at the natural 7 px spacing while they
// fit, then squeezed down to 1 px; `shown` is how many icons fit (the rest are not drawn).
void getTreasureLayout(int coins, int &shown, int &itemsPerRow, float &colsepar);

// How much of `resourceId` a buildable's recipe consumes, independent of what the city has
// in stock: hands fullfillment() an abundance of every id getRequiredResources() lists and
// reads the deduction back. 0 if the recipe never consumes that id.
int factoryRequirement(class BuildableFactory* bf, int resourceId);

// Units currently standing on city's tile, in a stable order shared by drawCityScreen (to
// list them) and clickOnCityScreen (to map a clicked row back to the same unit).
std::vector<Unit*> getUnitsAtCity(City* city);

// Resources actually in storage (city->resources[id]>0 then city->resources[id]>0), in a
// stable order -- stocked commodities first (ALL_COMMODITIES order), then stocked mfg goods
// (ALL_MFG_GOODS order) -- shared by drawCityScreen (the "Resource Storage" box) and
// clickOnCityScreen (its "load" arrow).
std::vector<int> getStockedResources(City* city);

void initCoreResources();

#endif   // CITYSCREENUI_H