//  TestCase_045.cpp
//  bunmei
//
//  Created by Claude on 03/09/2026
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
#include "../buildings/Factory.h"

#include "testcase_045.h"

// @Task #37 follow-up: a Factory that consumes the `iron` commodity and produces the `tools`
// mfg good. Verifies the year-by-year production step (engine.cpp operateCityBuildings(),
// called from bunmei.cpp endOfYear()) AND that the resulting mfg goods show up in the city
// UI's "Resource Storage" list:
//   1. Each operate() cycle: tools += Factory::getProductionRate(tools) (== 1),
//      iron -= Factory::getConsumptionRate(iron) (== 1). This pins the production-gate bug
//      that hid the feature -- the loop that adds output used to test getConsumptionRate(id)
//      (0 for tools, the Factory's OUTPUT) instead of getProductionRate(id), so tools were
//      never added and never appeared in the city.
//   2. Once tools > 0, getStockedResources() (the row list drawCityScreen() renders, mfg
//      goods after commodities) includes `tools`, and drawCityScreen() renders it without
//      crashing (exercises the tiles[tools] icon lookup).
//   3. When the iron stock is too low to cover one cycle's consumption, the Factory produces
//      nothing and consumes nothing that year (all-or-nothing on inputs).
// Also renders the "City Production" box (drawCityScreen) with the Factory both starved
// (red "/" divider) and supplied (white "/" divider) as a crash/sanity check on that new
// per-building "consumed commodity / produced mfg good" row.

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

TestCase_045::TestCase_045()
{

}

TestCase_045::~TestCase_045()
{

}

int TestCase_045::number()
{
    return 45;
}

void TestCase_045::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon) = mapcell(LAND);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon).setVisible(0);

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255;
    faction->green = 0;
    faction->blue = 0;
    faction->autoPlayer = false;

    factions.push_back(faction);

    // Not at (0,0): same reasoning as testcase_042/044 -- drawCityScreen is normally called
    // with the city's real (nonzero) screen position, so a city at true (0,0) would mask
    // drawing code that forgets to add cla/clo to its own coordinates.
    City *city = new City(&map, 0, getNextCityId(), 3, 3);
    city->setName("Kattegate");
    city->foundedyear = -4000;
    city->commodities[iron] = 3;                 // exactly 3 production cycles' worth
    city->buildings.push_back(new Factory());    // consumes iron, produces tools
    cities[city->id] = city;
    cityid = city->id;

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
}

int TestCase_045::check(int year)
{
    ticks++;

    if (isdone)
        return 0;

    // tester.cpp resets controller.view=1 right after init() -- re-set it every tick (the
    // testcase_025/029/042/044 workaround) so the city screen is actually open for the render.
    controller.view = 2;
    controller.cityid = cityid;

    if (ticks < 3)
        return 0;

    City* city = cities[cityid];

    // --- 1) Three good production cycles: tools +1, iron -1 each. ---
    for (int cycle = 1; cycle <= 3; cycle++)
    {
        operateCityBuildings(city);

        if (city->mfggoods[tools] != cycle)
        {
            isdone = true; haspassed = false;
            char buf[256];
            snprintf(buf,sizeof(buf),
                "After %d cycle(s) mfggoods[tools]=%d, expected %d (production gate bug: output must key on getProductionRate, not getConsumptionRate).",
                cycle, city->mfggoods[tools], cycle);
            message = std::string(buf);
            return 0;
        }
        if (city->commodities[iron] != 3 - cycle)
        {
            isdone = true; haspassed = false;
            char buf[256];
            snprintf(buf,sizeof(buf),"After %d cycle(s) commodities[iron]=%d, expected %d.",
                     cycle, city->commodities[iron], 3 - cycle);
            message = std::string(buf);
            return 0;
        }
    }

    // --- 2) tools now show in the city UI's Resource Storage list (mfg goods after
    //        commodities), and the screen renders with a tools icon without crashing. ---
    std::vector<int> stocked = getStockedResources(city);
    if (std::find(stocked.begin(), stocked.end(), (int)tools) == stocked.end())
    {
        isdone = true; haspassed = false;
        message = std::string("getStockedResources() does not list `tools` even though city->mfggoods[tools] > 0.");
        return 0;
    }
    // iron ran out on the 3rd cycle (0 left) -> must NOT be listed anymore.
    if (std::find(stocked.begin(), stocked.end(), (int)iron) != stocked.end())
    {
        isdone = true; haspassed = false;
        message = std::string("getStockedResources() still lists `iron` at 0 stock.");
        return 0;
    }

    // City Production box with the Factory starved (iron == 0 -> red "/" divider row).
    coordinate c = map.to_screen(city->latitude, city->longitude);
    drawCityScreen(c.lat, c.lon, city);

    // --- 3) Out of iron: a cycle now produces and consumes nothing (all-or-nothing). ---
    int toolsBefore = city->mfggoods[tools];
    operateCityBuildings(city);
    if (city->mfggoods[tools] != toolsBefore || city->commodities[iron] != 0)
    {
        isdone = true; haspassed = false;
        char buf[256];
        snprintf(buf,sizeof(buf),
            "Factory ran with no iron: mfggoods[tools]=%d (expected %d), commodities[iron]=%d (expected 0).",
            city->mfggoods[tools], toolsBefore, city->commodities[iron]);
        message = std::string(buf);
        return 0;
    }

    // --- Restock iron: production resumes. ---
    city->commodities[iron] = 5;
    operateCityBuildings(city);
    if (city->mfggoods[tools] != toolsBefore + 1 || city->commodities[iron] != 4)
    {
        isdone = true; haspassed = false;
        char buf[256];
        snprintf(buf,sizeof(buf),
            "Factory did not resume after restock: mfggoods[tools]=%d (expected %d), commodities[iron]=%d (expected 4).",
            city->mfggoods[tools], toolsBefore + 1, city->commodities[iron]);
        message = std::string(buf);
        return 0;
    }

    // City Production box with the Factory supplied (iron == 4 -> white "/" divider row).
    drawCityScreen(c.lat, c.lon, city);

    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_045::title()
{
    return std::string("Factory consumes iron and produces tools each year (operateCityBuildings); tools then show in the city UI Resource Storage list.");
}

bool TestCase_045::done()
{
    return isdone;
}
bool TestCase_045::passed()
{
    return haspassed;
}
std::string TestCase_045::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_045();
}
