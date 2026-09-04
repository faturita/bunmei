#ifndef MARKETUI_H
#define MARKETUI_H

#include <unordered_map>

// The market screen (controller.view == 7, toggled with '&'): a drawInfoScreen-style
// full-screen table listing every shippable resource (commodities then mfg goods), its
// current unit price (prices[], tiles.cpp) and the total amount of it held across every
// city the viewing faction can currently see.
void drawMarketScreen();

// resource id -> total units of that shippable resource stocked across every city whose map
// tile isVisible() for the given faction (own or foreign -- visibility, not ownership).
// Every ALL_COMMODITIES / ALL_MFG_GOODS id is present (0 if none). Exposed so the testcase
// can check the summing independent of rendering, same as infoui.cpp:getCitiesForFaction().
std::unordered_map<int,int> getShippableStockForFaction(int faction_id);

#endif   // MARKETUI_H
