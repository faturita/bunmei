#include <unordered_map>
#include <vector>
#include <cstdio>
#include <queue>

#include "map.h"
#include "resources.h"
#include "City.h"
#include "Faction.h"
#include "tiles.h"
#include "coordinator.h"
#include "usercontrols.h"
#include "commandorder.h"
#include "cityscreenui.h"
#include "units/Unit.h"
#include "units/Transport.h"
#include "font/FontsBitmap.h"
#include "commerceui.h"

extern Map map;
extern Controller controller;
extern Coordinator coordinator;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;

// Own scroll offset for the "For sale" box (the commerce-screen counterpart of
// cityscreenui.cpp's commoditiesStorageOffset).
static int commerceStorageOffset = 0;

// The Transport (as a Unit, for id/faction) whose faction opened the commerce screen.
static Unit* tradingUnit()
{
    auto it = units.find(controller.tradeunitid);
    if (it == units.end()) return nullptr;
    return (dynamic_cast<Transport*>(it->second) != nullptr) ? it->second : nullptr;
}

std::vector<int> getTradeCargo()
{
    std::vector<int> ids;
    Unit* u = tradingUnit();
    if (u == nullptr) return ids;
    for (Shippable* s : dynamic_cast<Transport*>(u)->getCargo())
        ids.push_back(s->getId());
    return ids;
}

void drawCommerceScreen(int cla, int clo, City *city)
{
    Unit* u = tradingUnit();

    // A stripped city screen (issue.png): same footprint and background tile, but only the
    // bottom-left "resources accumulated" box and the bottom-centre "port" box are drawn --
    // both rendered with the very same code the city screen uses.
    for (int lats = -10; lats < 10; lats++)
        for (int lons = -10; lons < 10; lons++)
            placeTile(clo + lons, cla + lats, "assets/assets/general/citytexture.png");

    drawBoundingBox(clo, cla, -10, -10, 9, 9);

    placeWord(clo + (-10), cla + (-10), 4, 8, city->name);

    char buf[128];
    int youcoins = (u != nullptr && u->faction >= 0 && u->faction < (int)factions.size())
                   ? factions[u->faction]->coins : 0;
    snprintf(buf, sizeof(buf), "COMMERCE   you %d coins   %s %d coins",
             youcoins, city->name, city->coreresources[COINS]);
    placeWord(clo + (-10), cla + (-9), 4, 8, buf);

    // Left: the city's stocked resources, drawn exactly like the city screen's
    // "Resource Storage" box (icon strip, 1 icon / 10 units), but the per-row arrow BUYS
    // (clickOnCommerceScreen -> Command::BuyResourceOrder).
    drawResourceStorageBox(cla, clo, city, commerceStorageOffset,
                           "For sale", "assets/assets/cursor/right.png");

    // Centre: the trading Transport in a "port" box, drawn as a city-screen Units-box row
    // (unit icon + box.png cargo slots). Press a box.png slot to SELL that stack.
    placeWord(clo + (-3), cla + (4), 4, 8, "Port");
    drawBoundingBox(clo, cla, -3, 4, 3, 9);
    if (u != nullptr)
        drawUnitsBoxRow(cla, clo, u, 0);
}

void clickOnCommerceScreen(int lat, int lon, int lat2, int lon2)
{
    auto cityIt = cities.find(controller.cityid);
    Unit* u = tradingUnit();
    if (cityIt == cities.end() || u == nullptr)
        return;

    // "For sale" box up/down scroll arrows -- same hit spots as the city screen's
    // Resource Storage box (lat2 10/18, lon2 -8).
    if (lat2==10 && lon2==-8) { commerceStorageOffset++; return; }
    if (lat2==18 && lon2==-8) { commerceStorageOffset--; return; }

    // "For sale" box per-row "buy" arrow (cursor/right.png, column -5): buys up to 100 of
    // that row's resource onto the trading Transport.
    if (lat>=5 && lat<=9 && lon==-5)
    {
        std::vector<int> stocked = getStockedResources(cityIt->second);
        int idx = (lat - 5) - commerceStorageOffset;
        if (idx >= 0 && idx < (int)stocked.size())
        {
            CommandOrder co;
            co.command = Command::BuyResourceOrder;
            co.parameters.spawnid = u->id;
            co.parameters.factionid = u->faction;
            co.parameters.cityid = controller.cityid;
            co.parameters.resourceid = stocked[idx];
            coordinator.push(co);
        }
        return;
    }

    // "Port" box: the trading Transport is the only row (lat==5). Pressing a box.png cargo
    // slot (slot s at fine grid lon2 == s-4) sells that stack -- the same mechanism the
    // city screen's Units box uses to unload a slot.
    if (lat>=5 && lat<=9 && lon>=-3 && lon<=3)
    {
        if (Transport* transport = dynamic_cast<Transport*>(u))
        {
            int slot = lon2 + 4;
            if (slot >= 0 && slot < transport->capacity())
            {
                std::vector<Shippable*> cargo = transport->getCargo();
                if (slot < (int)cargo.size())
                {
                    CommandOrder co;
                    co.command = Command::SellResourceOrder;
                    co.parameters.spawnid = u->id;
                    co.parameters.factionid = u->faction;
                    co.parameters.cityid = controller.cityid;
                    co.parameters.resourceid = cargo[slot]->getId();
                    coordinator.push(co);
                }
            }
        }
        return;
    }
}

void openCommerceScreen()
{
    if (controller.view != 4)
        return;

    auto it = cities.find(controller.cityid);
    if (it == cities.end())
        return;

    City* city = it->second;

    // Same placement/centering as openCityScreen(): pan the map so the traded city is at
    // screen centre, then draw the panel at that city's screen position, so the commerce
    // screen sits exactly where the city screen would.
    coordinate c = map.to_screen(city->latitude, city->longitude);
    centermapinmap(c.lat, c.lon);
    drawCommerceScreen(c.lat, c.lon, city);
}
