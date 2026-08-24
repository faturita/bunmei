//  TestCase_033.cpp
//  bunmei
//
//  Created by Claude on 24/08/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <map>
#include <iterator>

#include "../map.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../commandorder.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"

#include "testcase_033.h"

// Regression for the "Change" list bug reported via issue.png: opening a city's Change list
// (Command::PopulateBuildableOrder, engine.cpp:processCommandOrders) pushed the FULL catalog
// of Buildable*Factory instances into a function-local `static` list in
// populateCityBuildables() every single call, instead of only the first time -- so every
// re-open of the Change screen (a normal action: pick something to build, close, reopen next
// turn) appended another whole duplicate batch, and City::buildable ended up with the same
// items repeated once per Change-open. Additionally, WorkerFactory was pushed onto that
// catalog twice within a single populate, so even the very first open showed "Worker" twice.
//
// This test pushes Command::PopulateBuildableOrder for the same city three times in a row
// (simulating "open Change, open Change again, open Change a third time" -- exactly what
// happens across a few turns of picking production) and checks that city->buildable's size
// and contents are IDENTICAL every time (no growth), and that no factory name appears more
// than once in the result.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;
extern Tiles tiles;

extern float mapzoom;

extern Coordinator coordinator;

#define TEST_MAPSIZE 1

TestCase_033::TestCase_033()
{

}

TestCase_033::~TestCase_033()
{

}

int TestCase_033::number()
{
    return 33;
}

void TestCase_033::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);

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

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

}

// Pushes Command::PopulateBuildableOrder for the test city and drains it, same as opening the
// city UI's Change list does (cityscreenui.cpp clickOnCityScreen).
static void openChangeList(int cityid)
{
    CommandOrder co;
    co.command = Command::PopulateBuildableOrder;
    co.parameters.cityid = cityid;
    coordinator.push(co);
    processCommandOrders();
}

int TestCase_033::check(int year)
{

    ticks++;

    if (isdone)
        return 0;

    // Give the game loop a couple of ticks to settle, same as testcase_025/026/029/032.
    if (ticks < 3)
        return 0;

    City* city = cities[cityid];

    openChangeList(cityid);
    size_t firstSize = city->buildable.size();
    std::map<std::string, int> firstCounts;
    for (auto& bf : city->buildable)
        firstCounts[std::string(bf->name)]++;

    for (auto& [name, count] : firstCounts)
    {
        if (count > 1)
        {
            isdone = true;
            haspassed = false;
            message = std::string("Factory '") + name + "' already appears " + std::to_string(count) + " times after the FIRST Change-list open.";
            return 0;
        }
    }

    if (firstSize == 0)
    {
        isdone = true;
        haspassed = false;
        message = std::string("city->buildable is empty after opening the Change list -- expected at least the ungated units/buildings (Palace/Scout/Settler/Worker/Warrior).");
        return 0;
    }

    // Re-open the Change list twice more, same as picking something to build and coming back
    // in a later turn to pick something else.
    openChangeList(cityid);
    size_t secondSize = city->buildable.size();

    openChangeList(cityid);
    size_t thirdSize = city->buildable.size();

    if (secondSize != firstSize || thirdSize != firstSize)
    {
        isdone = true;
        haspassed = false;
        message = std::string("city->buildable grew across repeated Change-list opens: ")
            + std::to_string(firstSize) + " -> " + std::to_string(secondSize) + " -> " + std::to_string(thirdSize)
            + " (populateCityBuildables()'s master factory list must be built once, not re-appended every call).";
        return 0;
    }

    std::map<std::string, int> thirdCounts;
    for (auto& bf : city->buildable)
        thirdCounts[std::string(bf->name)]++;

    for (auto& [name, count] : thirdCounts)
    {
        if (count > 1)
        {
            isdone = true;
            haspassed = false;
            message = std::string("Factory '") + name + "' appears " + std::to_string(count) + " times after re-opening the Change list.";
            return 0;
        }
    }

    isdone = true;
    haspassed = true;

    return 0;
}
std::string TestCase_033::title()
{
    return std::string("Repeated Command::PopulateBuildableOrder does not duplicate/grow a city's buildable list (issue.png regression).");

}

bool TestCase_033::done()
{
    return isdone;
}
bool TestCase_033::passed()
{
    return haspassed;
}
std::string TestCase_033::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_033();
}
