//  TestCase_032.cpp
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

#include "testcase_032.h"

// City UI tile work assignment now goes through the Command pattern (@Task: "Add
// CommandOrders for all the assignment/deassignment of work inside the tiles of the city
// UI"). Previously, clicking a city tile (clickOnCityScreen) just set a flag
// (tileWorkingIsActive/clickedTile), and the NEXT drawCityScreen() render pass mutated the
// city directly (city->assignWorkingTile(clickedTile)) -- a game-state change made from
// inside the RENDER path, with no record of the action anywhere. Now drawCityScreen() only
// pushes a Command::AssignWorkTileOrder (cityscreenui.cpp), and engine.cpp's
// processCommandOrders() is the one place that actually calls
// City::assignWorkingTile(coordinate) (which itself decides assign vs. deassign from the
// tile's current state, same toggle semantics as before).
//
// This test clicks a non-center city tile, renders once (drawCityScreen) and checks the tile's
// working state has NOT changed yet (the render pass must only have pushed the command), then
// calls processCommandOrders() and checks the tile's state DID flip. It repeats the click to
// confirm the toggle also works in the other direction (deassign), through the same path.

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

#define TEST_MAPSIZE 1

// A non-center tile (city center (0,0) is never assignable, City.cpp assignWorkingTile).
#define TEST_TILE_LAT 1
#define TEST_TILE_LON 1

TestCase_032::TestCase_032()
{

}

TestCase_032::~TestCase_032()
{

}

int TestCase_032::number()
{
    return 32;
}

void TestCase_032::init()
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

int TestCase_032::check(int year)
{

    ticks++;

    if (isdone)
        return 0;

    // Give the game loop a couple of ticks to settle, same as testcase_025/026/029.
    if (ticks < 3)
        return 0;

    City* city = cities[cityid];
    coordinate c = map.to_screen(city->latitude, city->longitude);

    bool before = city->workingOn(TEST_TILE_LAT, TEST_TILE_LON);

    // Simulate clicking the tile in the city screen (same call clickOnCityScreen's own
    // tile-click branch handles).
    clickOnCityScreen(TEST_TILE_LAT, TEST_TILE_LON, 0, 0);

    // A render pass alone must NOT mutate the city anymore -- only push a CommandOrder.
    drawCityScreen(c.lat, c.lon, city);

    if (city->workingOn(TEST_TILE_LAT, TEST_TILE_LON) != before)
    {
        isdone = true;
        haspassed = false;
        message = std::string("drawCityScreen() changed the tile's working state directly -- it must only push a Command::AssignWorkTileOrder now.");
        return 0;
    }

    processCommandOrders();

    if (city->workingOn(TEST_TILE_LAT, TEST_TILE_LON) == before)
    {
        isdone = true;
        haspassed = false;
        message = std::string("processCommandOrders() did not toggle the tile's working state for Command::AssignWorkTileOrder.");
        return 0;
    }

    // Click the same tile again: same path must now DEASSIGN it (toggle back).
    clickOnCityScreen(TEST_TILE_LAT, TEST_TILE_LON, 0, 0);
    drawCityScreen(c.lat, c.lon, city);
    processCommandOrders();

    isdone = true;
    haspassed = (city->workingOn(TEST_TILE_LAT, TEST_TILE_LON) == before);
    if (!haspassed)
        message = std::string("Clicking the same tile again did not toggle the working state back.");

    return 0;
}
std::string TestCase_032::title()
{
    return std::string("City UI tile work assignment goes through Command::AssignWorkTileOrder / processCommandOrders(), not a direct mutation from drawCityScreen().");

}

bool TestCase_032::done()
{
    return isdone;
}
bool TestCase_032::passed()
{
    return haspassed;
}
std::string TestCase_032::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_032();
}
