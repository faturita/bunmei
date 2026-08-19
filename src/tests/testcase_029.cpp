//  TestCase_029.cpp
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
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../cityscreenui.h"

#include "testcase_029.h"

// City commodities in the city UI (task #13): the city screen gained a "City Commodities"
// box (per-turn gather rate, same "pack together" row style as the pre-existing "City
// Resources" box, cityscreenui.cpp drawCityScreen) and a "Commodities Storage" box (the
// accumulated stockpile, one row per commodity actually in storage, scrollable the SAME way
// as the "Units" box once more entries exist than fit -- cityscreenui.cpp's
// commoditiesStorageOffset). Making room for the storage box shrank "Food Storage" (rows
// -3..3 instead of -3..9), so its icon packing (foodItemsPerRow) is now computed from the
// smaller box instead of a flat 10-per-row.
//
// This test stations 7 commodities in a city's storage (more than COMMODITIES_STORAGE_ROWS,
// 5) and drives the SAME up/down arrow clicks the Units box already uses (lat2==10/18), at
// the Commodities Storage box's own column (lon2==-8, see cityscreenui.cpp for the
// derivation): checks the scroll offset clamps at both ends, mirroring testcase_026's
// coverage of the Units box scroll. It also exercises City::getCommodityProductionRate()
// through a real drawCityScreen() call (an ungated SILK tile and a Quarry-gated MARBLE tile,
// the latter without its Quarry) to make sure the render path does not crash when some
// commodities are gated to zero -- the gating logic itself is testcase_028's job.

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

// cityscreenui.cpp globals: not declared in cityscreenui.h (same as unitsOffset/
// selectionOffset), but still have external linkage, so a testcase can extern them directly.
extern int commoditiesStorageOffset;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1
#define NUMCOMMODITIES 7

TestCase_029::TestCase_029()
{

}

TestCase_029::~TestCase_029()
{

}

int TestCase_029::number()
{
    return 29;
}

void TestCase_029::init()
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

    // SILK needs no improvement: the "City Commodities" box must show it. MARBLE needs a
    // Quarry that is deliberately NOT built here: the render path must not crash just
    // because a commodity is gated to zero.
    map.set(1,0).bioma = GRASSLAND;
    map.set(1,0).resource = SILK;

    map.set(2,0).bioma = GRASSLAND;
    map.set(2,0).resource = MARBLE;

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon).setVisible(0);

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

    City *city = new City(&map, 0, getNextCityId(), 0, 0);
    city->setName("Kattegate");
    city->foundedyear = -4000;
    cities[city->id] = city;
    cityid = city->id;

    // A big food stockpile (more than the old flat 10-per-row would have fit in the new,
    // shorter box) and NUMCOMMODITIES (7) stocked commodities -- more than
    // COMMODITIES_STORAGE_ROWS (5), so the Commodities Storage box must scroll.
    city->resources[0] = 47;
    city->commodities[copper]    = 4;
    city->commodities[iron]      = 8;
    city->commodities[marble]    = 2;
    city->commodities[traan]     = 9;
    city->commodities[horses]    = 1;
    city->commodities[silk]      = 6;
    city->commodities[wine]      = 23;

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

}

int TestCase_029::check(int year)
{

    ticks++;

    if (isdone)
        return 0;

    // Give the game loop a couple of ticks to settle, same as testcase_025/026.
    if (ticks < 3)
        return 0;

    City* city = cities[cityid];

    coordinate c = map.to_screen(city->latitude, city->longitude);

    // A real render pass, including City Commodities (rate>0 SILK, rate==0 gated MARBLE)
    // and the new Commodities Storage list: must not crash.
    drawCityScreen(c.lat, c.lon, city);

    if (commoditiesStorageOffset != 0)
    {
        isdone = true;
        haspassed = false;
        message = std::string("Commodities Storage scroll offset did not start at 0.");
        return 0;
    }

    // Down arrow (lat2==18, lon2==-8, see cityscreenui.cpp for the derivation) repeatedly,
    // well past where it should clamp: with 7 commodities and 5 rows, the offset must stop
    // at -(7-5) = -2.
    for(int i=0;i<10;i++)
    {
        clickOnCityScreen(0, 0, 18, -8);
        drawCityScreen(c.lat, c.lon, city);
    }

    // 5 == COMMODITIES_STORAGE_ROWS (cityscreenui.cpp), same as testcase_026 hardcoding
    // UNITS_BOX_ROWS rather than externing a const.
    if (commoditiesStorageOffset != -(NUMCOMMODITIES-5))
    {
        isdone = true;
        haspassed = false;
        message = std::string("Commodities Storage down-scroll did not clamp at the last page.");
        return 0;
    }

    // Up arrow (lat2==10, lon2==-8) repeatedly, well past 0: must clamp back to 0.
    for(int i=0;i<10;i++)
    {
        clickOnCityScreen(0, 0, 10, -8);
        drawCityScreen(c.lat, c.lon, city);
    }

    isdone = true;

    haspassed = (commoditiesStorageOffset == 0);
    if (!haspassed)
        message = std::string("Commodities Storage up-scroll did not clamp back to 0.");

    return 0;
}
std::string TestCase_029::title()
{
    return std::string("City UI commodities: City Commodities/Commodities Storage boxes render and the storage list scrolls/clamps like the Units box.");

}

bool TestCase_029::done()
{
    return isdone;
}
bool TestCase_029::passed()
{
    return haspassed;
}
std::string TestCase_029::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_029();
}
