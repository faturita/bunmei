//  TestCase_053.cpp
//  bunmei
//
//  Created by Claude on 06/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <algorithm>

#include "../map.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../cityscreenui.h"
#include "../buildings/Building.h"
#include "../buildings/Palace.h"
#include "../buildings/Granary.h"
#include "../buildings/Market.h"
#include "../buildings/Factory.h"

#include "testcase_053.h"

// @Task: buildings consume core resources from the city (Building::getConsumptionRate, e.g.
// 1 coin per generic building; a Factory consumes iron instead). This checks the accounting
// the city UI now shows: City::getBuildingConsumptionRate(r) totals it per resource,
// SEPARATE from getConsumptionRate() (pop/tile upkeep) so endOfYear's operateCityBuildings()
// deduction is not double-counted. Plus a drawCityScreen() render (the "City Resources"
// box's consumed/gap/net rows and the per-building consumed-resource icons must not crash).

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Tiles tiles;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_053::TestCase_053() {}
TestCase_053::~TestCase_053() {}

int TestCase_053::number()
{
    return 53;
}

void TestCase_053::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initCoreResources();

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon) = mapcell(LAND);
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon).setVisible(0);

    Faction *faction = new Faction();
    faction->id = 0; strcpy(faction->name,"Vikings");
    faction->red = 255; faction->green = 0; faction->blue = 0;
    faction->autoPlayer = false;
    factions.push_back(faction);

    City *city = new City(&map, 0, getNextCityId(), 3, 3);
    city->setName("Kattegate");
    city->foundedyear = -4000;
    city->coreresources[COINS] = 50;
    city->commodities[iron] = 5;

    // Palace + Granary: generic Buildings -> 1 coin each. Factory: overrides -> 1 iron, 0 coin.
    city->buildings.push_back(new Palace());
    city->buildings.push_back(new Granary());
    city->buildings.push_back(new Factory());

    cities[city->id] = city;
    cityid = city->id;

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
}

int TestCase_053::check(int year)
{
    ticks++;
    if (isdone) return 0;

    controller.view = 2;
    controller.cityid = cityid;

    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    City* city = cities[cityid];

    // The checks + the one-off Market add below must run EXACTLY once; the render sanity
    // pass then happens on the following tick.
    if (phase == 1)
    {
        coordinate c = map.to_screen(city->latitude, city->longitude);
        drawCityScreen(c.lat, c.lon, city);   // City Resources consumed/gap/net + per-building icons, must not crash
        isdone = true;
        haspassed = true;
        return 0;
    }
    phase = 1;

    // 1) Building coin upkeep = the two generic buildings (Factory's override consumes iron,
    //    not coins).
    if (city->getBuildingConsumptionRate(COINS) != 2)
    {
        char buf[128];
        snprintf(buf,sizeof(buf),"getBuildingConsumptionRate(COINS)=%d, expected 2 (Palace + Granary).",
                 city->getBuildingConsumptionRate(COINS));
        fail(buf);
        return 0;
    }
    if (city->getBuildingConsumptionRate(iron) != 1)
    {
        fail("getBuildingConsumptionRate(iron) is not 1 (the Factory).");
        return 0;
    }
    // A resource nobody consumes.
    if (city->getBuildingConsumptionRate(SHIELDS) != 0 || city->getBuildingConsumptionRate(silver) != 0)
    {
        fail("getBuildingConsumptionRate reported upkeep for a resource no building consumes.");
        return 0;
    }

    // 2) getConsumptionRate() is UNCHANGED -- pop upkeep only, no building coins folded in
    //    (endOfYear deducts those via operateCityBuildings; folding them here would double).
    if (city->getConsumptionRate(COINS) != 0)
    {
        fail("getConsumptionRate(COINS) is non-zero -- building upkeep must NOT be folded into it.");
        return 0;
    }
    if (city->getConsumptionRate(FOOD) != city->pop * 2)
    {
        fail("getConsumptionRate(FOOD) changed -- should still be pop*2.");
        return 0;
    }

    // 3) Building a 4th generic building raises the coin upkeep to 3.
    city->buildings.push_back(new Market());
    if (city->getBuildingConsumptionRate(COINS) != 3)
    {
        fail("Adding a Market did not raise getBuildingConsumptionRate(COINS) to 3.");
        return 0;
    }

    // The City Resources box (consumed/gap/net) + per-building consumed-resource icons get a
    // render sanity pass on the next tick(s) via `phase == 1` above.
    return 0;
}

std::string TestCase_053::title()
{
    return std::string("Buildings consume core resources: City::getBuildingConsumptionRate totals it per resource (coins etc.), kept separate from pop upkeep; city UI renders the consumed/net split and per-building icons.");
}

bool TestCase_053::done()   { return isdone; }
bool TestCase_053::passed() { return haspassed; }
std::string TestCase_053::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_053();
}
