//  TestCase_034.cpp
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
#include "../cityscreenui.h"

#include "testcase_034.h"

// Regression for issue2.png: the Change list showed a blank gap before its first item
// (Palace/Scout/Settler/Worker/Warrior appearing bottom-justified in the 9-slot box instead
// of top-justified), even though the list itself was correct (no duplicates, task #23).
//
// Root cause (cityscreenui.cpp drawCityScreen, the Change-list scroll clamp): `int max =
// buildable.size()-slots;` is negative whenever the list is SHORTER than the box (the now-
// common case, since task #23 removed the duplicate-growth bug). The very next check,
// `if (selectionOffset<-max) selectionOffset=(size-slots)*(-1);`, then flips sign on that
// negative `max` and forces selectionOffset to a POSITIVE value (slots-size), pushing every
// item's on-screen row (`loc = i+selectionOffset`) down by that many slots -- exactly the
// blank-gap-then-bottom-justified list from the screenshot. The clamp is only meant to stop
// scrolling PAST the end of a list LONGER than the box; it was never guarded for the
// size<slots case.
//
// This test pushes Command::PopulateBuildableOrder (giving a short, un-gated list: Palace/
// Scout/Settler/Worker/Warrior -- 5 items in a 9-slot box), opens the Change list the same way
// the city UI does (changeIsActive=true), renders one frame (drawCityScreen, which is where
// the clamp lives), and checks selectionOffset stays 0 -- i.e. the list stays top-justified.

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

// cityscreenui.cpp globals driving the Change list (extern, same as the up/down-arrow
// scrolling state the UI itself mutates).
extern bool changeIsActive;
extern int selectionOffset;

#define TEST_MAPSIZE 1

TestCase_034::TestCase_034()
{

}

TestCase_034::~TestCase_034()
{

}

int TestCase_034::number()
{
    return 34;
}

void TestCase_034::init()
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

    controller.cityid = city->id;
    controller.view = 2;

}

int TestCase_034::check(int year)
{

    ticks++;

    if (isdone)
        return 0;

    // Give the game loop a couple of ticks to settle, same as testcase_025/026/029/032/033.
    if (ticks < 3)
        return 0;

    City* city = cities[cityid];

    // Open the Change list, same as Command::PopulateBuildableOrder does when the player
    // clicks it (cityscreenui.cpp).
    CommandOrder co;
    co.command = Command::PopulateBuildableOrder;
    co.parameters.cityid = cityid;
    coordinator.push(co);
    processCommandOrders();

    if (city->buildable.size() == 0 || city->buildable.size() >= 9)
    {
        isdone = true;
        haspassed = false;
        message = std::string("Expected a SHORT buildable list (<9 items, the un-gated units: Palace/Scout/Settler/Worker/Warrior) to reproduce the size<slots clamp bug; got ") + std::to_string(city->buildable.size()) + " items.";
        return 0;
    }

    changeIsActive = true;
    selectionOffset = 0;

    coordinate c = map.to_screen(city->latitude, city->longitude);
    drawCityScreen(c.lat, c.lon, city);

    isdone = true;
    haspassed = (selectionOffset == 0);
    if (!haspassed)
        message = std::string("selectionOffset was pushed to ") + std::to_string(selectionOffset) + " for a list shorter than the box -- the Change list is bottom-justified with a blank gap on top instead of starting at the first slot (issue2.png).";

    return 0;
}
std::string TestCase_034::title()
{
    return std::string("A city buildable list shorter than the Change box's slot count stays top-justified (issue2.png regression).");

}

bool TestCase_034::done()
{
    return isdone;
}
bool TestCase_034::passed()
{
    return haspassed;
}
std::string TestCase_034::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_034();
}
