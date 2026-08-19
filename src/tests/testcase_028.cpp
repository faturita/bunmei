//  TestCase_028.cpp
//  bunmei
//
//  Created by Claude on 19/08/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <deque>
#include <iterator>
#include <iostream>

#include "../map.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../map.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"

#include "testcase_028.h"

// City commodities (task #12): each year, endOfYear() (bunmei.cpp) now also gathers one unit
// of a commodity per matching special resource found within a city's 7x7 range (SAME bounds
// as City::getProductionRate, but -- unlike it -- NOT filtered by City::workingOn(): every
// tile in range counts, worked or not, per the task). City::getCommodityProductionRate()
// (City.cpp) does the counting: for each tile with a resource, look up its commodity
// (commodityxresource, tiles.cpp) and, if that resource needs an improvement
// (getRequiredImprovement/initImprovementResources), only count it if the tile has that
// improvement built (mapcell::improvements bit test) -- otherwise the resource contributes
// zero until the improvement exists.
//
// This test places a City with two special-resource tiles in range: SILK (no improvement
// required, produces its commodity unconditionally) and MARBLE (requires a Quarry). No
// worker/BuildQuarryOrder machinery is exercised here (that is testcase_027's job) -- the
// Quarry is flipped on directly via mapcell::buildQuarry() to isolate the production-gating
// logic in City::getCommodityProductionRate() itself.
//
// With a single faction and NO units at all, noMoreMovementsLeft() is trivially true every
// tick, so endOfYear() fires on essentially every tick of the tester's loop (see
// engine.cpp's endOfTurnForAllFactions()/Faction::ready()) -- years advance very fast, so
// checks use inequalities against a captured checkpoint rather than exact tick/year math.

extern Map map;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;
extern Tiles tiles;
extern MovementCost movementcosts;
extern ImprovementEffort improvementeffort;
extern ImprovementResources improvementresources;
extern std::unordered_map<int, int> commodityxresource;
extern bool autoEndOfTurn;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_028::TestCase_028()
{

}

TestCase_028::~TestCase_028()
{

}

int TestCase_028::number()
{
    return 28;
}

void TestCase_028::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initMovementCosts(movementcosts);
    initImprovementEffort(improvementeffort);
    initImprovementResources(improvementresources);
    initCommodities(commodityxresource);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }

    // A small island around the map center, where the city will sit.
    for (int lat=-8;lat<=8;lat++)
    {
        for (int lon=-8;lon<=8;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
        }
    }

    // SILK needs no improvement (README.md's resource table): produces its commodity as
    // soon as it is in range.
    map.set(1,0).bioma = GRASSLAND;
    map.set(1,0).resource = SILK;

    // MARBLE needs a Quarry: must produce nothing until one is built on this tile.
    map.set(2,0).bioma = GRASSLAND;
    map.set(2,0).resource = MARBLE;

    resources.push_back(new Resource(0,0,"assets/assets/city/food.png","Food"));
    resources.push_back(new Resource(1,0,"assets/assets/city/production.png","Shields"));
    resources.push_back(new Resource(2,0,"assets/assets/city/trade.png","Trade"));
    resources.push_back(new Resource(3,0,"assets/assets/city/gold.png","Coins"));
    resources.push_back(new Resource(4,0,"assets/assets/city/bulb.png","Science"));
    resources.push_back(new Resource(5,0,"assets/assets/city/culture.png","Culture"));

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            for(auto &r:resources)
            {
                map.set(lat,lon).addResourceProductionRate(2);
            }
        }

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255;
    faction->green = 0;
    faction->blue = 0;
    faction->autoPlayer = false;

    factions.push_back(faction);

    // A single faction with NO units: nothing ever has moves left, so noMoreMovementsLeft()
    // is trivially true and (with autoEndOfTurn) every tick ends the year -- see the header
    // comment above.
    autoEndOfTurn = true;

    City *city = new City(&map, 0, getNextCityId(), 0, 0);
    city->setName("Kattegate");
    cities[city->id] = city;
    cityid = city->id;

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 0;

}

int TestCase_028::check(int year)
{

    ticks++;

    City *c = cities[cityid];

    // MARBLE must never contribute anything before the Quarry exists, no matter how many
    // years have passed.
    if (ticks < 20 && c->commodities[marble] != 0)
    {
        isdone = true;
        haspassed = false;
        message = std::string("Marble commodity was produced without a Quarry on the tile.");
        return 0;
    }

    // Enough years should have passed by now for SILK (ungated) to have accumulated at
    // least once; capture it as a checkpoint and build the Quarry for the next phase.
    if (ticks == 20)
    {
        if (c->commodities[silk] < 1)
        {
            isdone = true;
            haspassed = false;
            message = std::string("Silk commodity was never gathered even though it needs no improvement.");
            return 0;
        }

        silkAtCheckpoint = c->commodities[silk];

        map.set(2,0).buildQuarry();
    }

    // Enough years have now passed since the Quarry was built: both commodities must have
    // grown -- marble because the gate is now open, silk because it keeps accumulating
    // regardless (the whole point of "regardless if worked or not").
    if (ticks == 40)
    {
        isdone = true;

        if (c->commodities[marble] < 1)
        {
            haspassed = false;
            message = std::string("Marble commodity was never gathered after the Quarry was built.");
            return 0;
        }

        if (c->commodities[silk] <= silkAtCheckpoint)
        {
            haspassed = false;
            message = std::string("Silk commodity stopped accumulating.");
            return 0;
        }

        haspassed = true;
    }

    return 0;
}
std::string TestCase_028::title()
{
    return std::string("City commodities: gathered from every special resource in range regardless of worked status, gated by required improvements.");

}

bool TestCase_028::done()
{
    return isdone;
}
bool TestCase_028::passed()
{
    return haspassed;
}
std::string TestCase_028::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_028();
}
