//  TestCase_043.cpp
//  bunmei
//
//  Created by Claude on 31/08/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <algorithm>

#include "../map.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../infoui.h"

#include "testcase_043.h"

// @Task: Information Screen (view=3, drawScene()'s switch, bunmei.cpp): a civscreen.png-style
// bordered full-screen panel (infoui.cpp/infoui.h) listing the viewed faction's cities with
// their core resource production rates. Exercises getCitiesForFaction() -- the selection
// logic drawInfoScreen() lists by -- directly, since a rendered screen can't be inspected by
// a testcase, then does a sanity render (must not crash) with the screen actually open.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Tiles tiles;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_043::TestCase_043()
{

}

TestCase_043::~TestCase_043()
{

}

int TestCase_043::number()
{
    return 43;
}

void TestCase_043::init()
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

    Faction *vikings = new Faction();
    vikings->id = 0;
    strcpy(vikings->name,"Vikings");
    vikings->red = 255;
    vikings->green = 0;
    vikings->blue = 0;
    vikings->autoPlayer = false;
    factions.push_back(vikings);

    Faction *mongols = new Faction();
    mongols->id = 1;
    strcpy(mongols->name,"Mongols");
    mongols->red = 0;
    mongols->green = 255;
    mongols->blue = 0;
    mongols->autoPlayer = false;
    factions.push_back(mongols);

    citynames[0] = std::queue<std::string>();
    citynames[1] = std::queue<std::string>();

    // Two cities for the viewed faction (0), one for another faction (1) -- far enough apart
    // that their worked-tile rings (City's 3-tile radius) don't overlap.
    City *cityA = new City(&map, 0, getNextCityId(), 3, 3);
    cityA->setName("Kattegate");
    cities[cityA->id] = cityA;
    cityAid = cityA->id;

    City *cityB = new City(&map, 0, getNextCityId(), -3, -3);
    cityB->setName("Uppsala");
    cities[cityB->id] = cityB;
    cityBid = cityB->id;

    City *cityC = new City(&map, 1, getNextCityId(), 8, 8);
    cityC->setName("Karakorum");
    cities[cityC->id] = cityC;
    cityCid = cityC->id;

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.v_f_id = 0;
}

int TestCase_043::check(int year)
{
    ticks++;

    if (isdone)
        return 0;

    // tester.cpp's initWorldModelling() runs right after init() and unconditionally resets
    // controller.view=1 -- setting view=3 here (every tick, not just once) is the established
    // workaround (testcase_025/026/029/042) for a test that needs a screen actually open,
    // rather than in init() where it would just get clobbered.
    controller.view = 3;

    // Give the game loop a few ticks to settle, same as testcase_025/026/029/032/042.
    if (ticks < 3)
        return 0;

    std::vector<City*> vikingCities = getCitiesForFaction(0);
    if (vikingCities.size() != 2)
    {
        isdone = true; haspassed = false;
        message = std::string("getCitiesForFaction(0) did not return exactly the 2 Viking cities.");
        return 0;
    }
    bool hasA = std::any_of(vikingCities.begin(), vikingCities.end(), [this](City* c){ return c->id == cityAid; });
    bool hasB = std::any_of(vikingCities.begin(), vikingCities.end(), [this](City* c){ return c->id == cityBid; });
    if (!hasA || !hasB)
    {
        isdone = true; haspassed = false;
        message = std::string("getCitiesForFaction(0) is missing Kattegate and/or Uppsala.");
        return 0;
    }

    std::vector<City*> mongolCities = getCitiesForFaction(1);
    if (mongolCities.size() != 1 || mongolCities[0]->id != cityCid)
    {
        isdone = true; haspassed = false;
        message = std::string("getCitiesForFaction(1) did not return exactly Karakorum.");
        return 0;
    }

    // Sanity render with the screen actually populated: must not crash.
    drawInfoScreen();

    isdone = true;
    haspassed = true;

    return 0;
}
std::string TestCase_043::title()
{
    return std::string("Information screen (view=3): getCitiesForFaction() filters by faction, drawInfoScreen() renders without crashing.");
}

bool TestCase_043::done()
{
    return isdone;
}
bool TestCase_043::passed()
{
    return haspassed;
}
std::string TestCase_043::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_043();
}
