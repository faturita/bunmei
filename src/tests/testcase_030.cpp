//  TestCase_030.cpp
//  bunmei
//
//  Created by Claude on 23/08/2026
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
#include "../units/Warrior.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../map.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../cityscreenui.h"

#include "testcase_030.h"

// Bug report (PROJECT.md @TASKS): on the city screen's "Change" (buildable) list, pressing
// the up arrow to scroll works the first time, but a later press sometimes selects a list
// item instead of scrolling.  Root cause (cityscreenui.cpp clickOnCityScreen): the up/down
// arrows sit at lat2==10/lat2==18, the EXACT SAME row as the list's first/last visible item
// (both boxes share the same row addressing, only the column -- lon2==18 -- distinguishes an
// arrow click from an item click).  The item-selection fallback branch didn't check the
// column at all, so a click that misses the arrow's exact row by a pixel (very plausible: the
// arrow icon is only ~8px tall) but still lands in the arrow's column fell through to
// selecting whatever item happens to share that row -- silently overwriting the city's
// production queue instead of just failing to scroll.
//
// This test builds a city with more buildable items than fit (so the arrows are relevant),
// opens "Change", and checks: (1) a near-miss click in the arrow's column but on the WRONG
// row does nothing (no selection, no scroll -- this is the exact click that reproduced the
// bug before the fix), (2) precise clicks on the arrows still scroll correctly (regression
// check), and (3) a genuine item click (different column) still selects and enqueues the
// right buildable (regression check for the normal path).

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;
extern Tiles tiles;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

// Change list's scroll offset (cityscreenui.cpp), not exposed via cityscreenui.h.
extern int selectionOffset;

#define NUMBUILDABLE 10

TestCase_030::TestCase_030()
{

}

TestCase_030::~TestCase_030()
{

}

int TestCase_030::number()
{
    return 30;
}

void TestCase_030::init()
{

    map.init(MAPHALFHEIGHT,MAPHALFWIDTH);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }

    // A small island surrounded by ocean.
    for (int lat=-3;lat<=3;lat++)
    {
        for (int lon=-3;lon<=3;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
        }
    }

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

    // A city at the map center, with more buildable items (NUMBUILDABLE=10) than the
    // Change list's slots (9, cityscreenui.cpp) can show at once, so the up/down arrows
    // are relevant.
    {
        City *city = new City(&map, 0, getNextCityId(), 0, 0);
        city->setName("Kattegate");
        city->foundedyear = -4000;
        for(int i=0;i<NUMBUILDABLE;i++)
            city->buildable.push_back(new WarriorFactory());
        cities[city->id] = city;
        cityid = city->id;
    }

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

}

int TestCase_030::check(int year)
{

    ticks++;

    if (isdone)
        return 0;

    // Give the game loop a couple of ticks to settle.
    if (ticks < 3)
        return 0;

    City* city = cities[cityid];

    controller.view = 2;
    controller.cityid = cityid;

    coordinate c = map.to_screen(city->latitude, city->longitude);

    // Open the "Change" list (row 4, column 4/5 -- see cityscreenui.cpp).  lat2/lon2 for
    // this click (0,0) land well outside the list/arrow area, so this only sets
    // changeIsActive without side effects.
    clickOnCityScreen(4, 5, 0, 0);
    drawCityScreen(c.lat, c.lon, city);

    if (!city->productionQueue.empty())
    {
        isdone = true;
        haspassed = false;
        message = std::string("Opening the Change list unexpectedly enqueued something.");
        return 0;
    }

    // The exact click that reproduced the reported bug: the up arrow's COLUMN (lon2==18)
    // but ONE ROW OFF (lat2==11 instead of 10) -- a plausible near-miss given the arrow icon
    // is only ~8px tall.  Before the fix, this fell through to the item-selection branch and
    // picked buildable[1]; it must now do nothing at all.
    clickOnCityScreen(0, 0, 11, 18);
    drawCityScreen(c.lat, c.lon, city);

    if (!city->productionQueue.empty())
    {
        isdone = true;
        haspassed = false;
        message = std::string("A near-miss click in the up arrow's column (wrong row) incorrectly selected a buildable item instead of being ignored.");
        return 0;
    }

    if (selectionOffset != 0)
    {
        isdone = true;
        haspassed = false;
        message = std::string("A near-miss click in the up arrow's column (wrong row) incorrectly scrolled the list.");
        return 0;
    }

    // Precise click on the DOWN arrow (lat2==18,lon2==18): must still scroll (regression
    // check that the fix didn't break the legitimate arrow click).
    clickOnCityScreen(0, 0, 18, 18);
    drawCityScreen(c.lat, c.lon, city);

    if (selectionOffset != -1)
    {
        isdone = true;
        haspassed = false;
        message = std::string("A precise click on the down arrow did not scroll the list.");
        return 0;
    }

    // Precise click on the UP arrow (lat2==10,lon2==18): must scroll back.
    clickOnCityScreen(0, 0, 10, 18);
    drawCityScreen(c.lat, c.lon, city);

    if (selectionOffset != 0)
    {
        isdone = true;
        haspassed = false;
        message = std::string("A precise click on the up arrow did not scroll the list back.");
        return 0;
    }

    // A genuine item click (row 2, well inside the text column, away from the arrows'
    // column) must still select and enqueue the right buildable (regression check for the
    // normal selection path).
    clickOnCityScreen(0, 0, 12, 8);
    drawCityScreen(c.lat, c.lon, city);

    isdone = true;
    haspassed = (!city->productionQueue.empty() && city->productionQueue.front() == city->buildable[2]);
    if (!haspassed)
        message = std::string("A genuine item click did not select and enqueue the right buildable item.");

    return 0;
}
std::string TestCase_030::title()
{
    return std::string("City screen Change list: a near-miss click in the arrow's column no longer mis-selects a buildable item.");

}

bool TestCase_030::done()
{
    return isdone;
}
bool TestCase_030::passed()
{
    return haspassed;
}
std::string TestCase_030::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_030();
}
