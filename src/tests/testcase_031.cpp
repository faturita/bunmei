//  TestCase_031.cpp
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
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../cityscreenui.h"

#include "testcase_031.h"

// Food Storage box (@Task: pack food icons more tightly, per City.cpp getPopulationThresshold()).
// City::resources[0] can climb all the way up to getPopulationThresshold(city->pop) (100*pop,
// bunmei.cpp endOfYear) right before the city grows, but the previous layout picked its
// items-per-row from the CURRENT stock and clamped it to whatever fit the box's fixed 7px
// spacing -- so once the stock got close to a large thresshold (a city with several
// population points), the grid needed more ROWS than the box has and icons spilled out the
// bottom. cityscreenui.cpp's new getFoodStorageLayout(pop, itemsPerRow, colsepar) instead
// sizes itemsPerRow off the thresshold (not the current stock, so the grid doesn't reshuffle
// as food accumulates) and squeezes the column spacing (colsepar) -- the same technique the
// City Resources/City Commodities boxes already use to pack a row -- to keep the whole grid
// inside the box for any population, however large.
//
// This test checks the MATH directly (no pixel capture needed): for a range of population
// values, itemsPerRow*rows must cover the thresshold, and the resulting row's pixel span
// (colsepar*(itemsPerRow-1) + icon width) must not exceed the box's actual pixel width. It
// then drives a real drawCityScreen() with a large population/food stock, matching this
// codebase's pattern of also exercising the real render path, to make sure it does not crash.
//
// @Issue follow-up: at a small thresshold (pop=1, 100 food), itemsPerRow was only 8 even
// though the row (same width as City Resources/Commodities, whose own comment says "16
// resources fit with a colsepar of 7") has room for double that -- verified with a real
// screen capture (glReadPixels + lodepng, tmp/ dir, add/use/removed) showing the row's icons
// stopping at half the box's width. getFoodStorageLayout now takes itemsPerRow as
// max(16, ceil(thresshold/rows)), so this test also checks itemsPerRow>=16 whenever the
// thresshold is small enough to allow it.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;
extern Tiles tiles;
extern MovementCost movementcosts;
extern ImprovementEffort improvementeffort;
extern ImprovementResources improvementresources;
extern std::unordered_map<int, int> commodityxresource;
extern std::unordered_map<int, Improvement*> improvements;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

// Same box geometry as cityscreenui.cpp's getFoodStorageLayout (Food Storage: cols -10..-4,
// rows -3..3).
#define FOOD_STORAGE_ROWS ( ((3)-(-3))*16/7 )
#define FOOD_STORAGE_WIDTH_PX ( ((-4)-(-10))*16 )
#define FOOD_ICON_PX 7

TestCase_031::TestCase_031()
{

}

TestCase_031::~TestCase_031()
{

}

int TestCase_031::number()
{
    return 31;
}

void TestCase_031::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initMovementCosts(movementcosts);
    initImprovementEffort(improvementeffort);
    initImprovementResources(improvementresources);
    initCommodities(commodityxresource);
    initImprovements(improvements);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }

    for (int lat=-8;lat<=8;lat++)
        for (int lon=-8;lon<=8;lon++)
            map.set(lat,lon) = mapcell(LAND);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon).setVisible(0);

    resources.push_back(new Resource(0,0,"assets/assets/city/food.png","Food"));
    resources.push_back(new Resource(1,0,"assets/assets/city/production.png","Shields"));
    resources.push_back(new Resource(2,0,"assets/assets/city/trade.png","Trade"));
    resources.push_back(new Resource(3,0,"assets/assets/city/gold.png","Coins"));
    resources.push_back(new Resource(4,0,"assets/assets/city/bulb.png","Science"));
    resources.push_back(new Resource(5,0,"assets/assets/city/culture.png","Culture"));

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255;
    faction->green = 0;
    faction->blue = 0;
    faction->autoPlayer = false;

    factions.push_back(faction);

    City *city = new City(&map, 0, getNextCityId(), 0, 0);
    city->setName("Kattegate");
    city->foundedyear = -4000;
    cities[city->id] = city;
    cityid = city->id;

    // A big population, so getPopulationThresshold(pop)=100*pop is large enough that the old
    // fixed-10-items-per-row (and later the items-per-row-clamped-to-the-box) layout would
    // need far more than the box's ~13 rows.
    city->pop = 25;
    city->resources[0] = getPopulationThresshold(city->pop); // right at the growth threshold

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

}

int TestCase_031::check(int year)
{

    ticks++;

    if (isdone)
        return 0;

    // Give the game loop a couple of ticks to settle, same as testcase_025/026/029.
    if (ticks < 3)
        return 0;

    City* city = cities[cityid];

    // Check the layout math for a spread of population values, including the city's own
    // (25) and some much larger ones, so the fix is verified beyond a single case.
    int testPops[] = {1, 2, 5, 10, 25, 50, 100};
    for (int p : testPops)
    {
        int itemsPerRow, colsepar;
        getFoodStorageLayout(p, itemsPerRow, colsepar);

        int thresshold = getPopulationThresshold(p);

        if (itemsPerRow < 16)
        {
            isdone = true;
            haspassed = false;
            char buf[256];
            sprintf(buf,"pop=%d: itemsPerRow(%d) is below the row's natural 16-item capacity.",
                    p, itemsPerRow);
            message = std::string(buf);
            return 0;
        }

        if ((long long)itemsPerRow * FOOD_STORAGE_ROWS < thresshold)
        {
            isdone = true;
            haspassed = false;
            char buf[256];
            sprintf(buf,"pop=%d: itemsPerRow(%d)*rows(%d) does not cover thresshold(%d).",
                    p, itemsPerRow, FOOD_STORAGE_ROWS, thresshold);
            message = std::string(buf);
            return 0;
        }

        int span = (itemsPerRow<=1) ? FOOD_ICON_PX : colsepar*(itemsPerRow-1) + FOOD_ICON_PX;
        if (span > FOOD_STORAGE_WIDTH_PX)
        {
            isdone = true;
            haspassed = false;
            char buf[256];
            sprintf(buf,"pop=%d: row span (%d px) overflows the Food Storage box (%d px).",
                    p, span, FOOD_STORAGE_WIDTH_PX);
            message = std::string(buf);
            return 0;
        }
    }

    coordinate c = map.to_screen(city->latitude, city->longitude);

    // A real render pass with a large population/food stockpile: must not crash.
    drawCityScreen(c.lat, c.lon, city);

    isdone = true;
    haspassed = true;

    return 0;
}
std::string TestCase_031::title()
{
    return std::string("Food Storage box: layout sized off getPopulationThresshold(pop) fits any population without overflowing the box.");

}

bool TestCase_031::done()
{
    return isdone;
}
bool TestCase_031::passed()
{
    return haspassed;
}
std::string TestCase_031::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_031();
}
