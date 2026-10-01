//  TestCase_086.cpp
//  bunmei
//
//  Created by faturita on 01/10/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <algorithm>

#include "../map.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../cityscreenui.h"

#include "testcase_086.h"

// The Treasure box (right column, between the buildings and the Change box) shows the coins
// stored in the city, city->resources[COINS]: one gold.png per coin, nogold.png when it is in
// debt. Icons sit at 7 px while they fit and are squeezed (getTreasureLayout) so that every
// one of them stays inside the box: 80 px wide, 6 rows.
//
// The layout is checked directly; the icon choice through the real drawCityScreen(): every
// icon is loaded into `maptextures` the first time it is drawn, and nothing else on this
// city's screen draws nogold.png (no coin shortage: no buildings).

extern Map map;
extern std::unordered_map<int, City*> cities;
extern Factions factions;
extern Tiles tiles;
extern std::unordered_map<std::string, GLuint> maptextures;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

#define NOGOLD "assets/assets/city/nogold.png"

TestCase_086::TestCase_086()
{

}

TestCase_086::~TestCase_086()
{

}

int TestCase_086::number()
{
    return 86;
}

static bool drawn(const char* icon)
{
    return maptextures.find(std::string(icon)) != maptextures.end();
}

void TestCase_086::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initCoreResources();

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
            map.set(lat,lon).bioma = GRASSLAND;
            map.set(lat,lon).setVisible(0);
        }
    assignProductionRates(map);     // tile yields from the terrain tables, as the game does

    Faction *faction = createFaction(0);
    faction->autoPlayer = false;
    factions.push_back(faction);

    City *city = new City(&map, 0, getNextCityId(), 3, 3);
    city->setName("Kattegate");
    city->setCityPop(1);
    city->assignWorkingTile();      // two grassland tiles feed the one pop: no shortage icon
    city->assignWorkingTile();
    city->foundedyear = -4000;
    cities[city->id] = city;
    cityid = city->id;

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

}

int TestCase_086::check(int year)
{

    ticks++;

    if (isdone)
        return 0;

    if (ticks < 3)
        return 0;

    isdone = true;
    haspassed = false;
    char buf[300];

    // ---- layout: every icon inside the box, natural spacing while it fits -------------------
    const int width = 80, rows = 6;
    for (int coins : { 0, 1, 5, 11, 12, 66, 67, 100, 300, 444, 445, 5000, -3, -200 })
    {
        int shown, perRow; float colsepar;
        getTreasureLayout(coins, shown, perRow, colsepar);
        int expected = std::min(std::abs(coins), 444);
        int lastx = (int)round(colsepar*(std::min(shown, perRow)-1)) + 7;
        int usedrows = (shown + perRow - 1)/perRow;
        if (shown != expected || lastx > width || usedrows > rows || colsepar < 1.0f || colsepar > 7.0f)
        {
            snprintf(buf, sizeof(buf), "%d coins: %d shown (expected %d), %d per row at %.2f px -> %d px wide (max %d), %d rows (max %d).",
                     coins, shown, expected, perRow, colsepar, lastx, width, usedrows, rows);
            message = std::string(buf);
            return 0;
        }
        if (std::abs(coins) <= 66 && colsepar != 7.0f)
        {
            snprintf(buf, sizeof(buf), "%d coins fit at 7 px, but were spaced %.2f px.", coins, colsepar);
            message = std::string(buf);
            return 0;
        }
    }

    // ---- icons: gold for a treasure, nogold for a debt ---------------------------------------
    City *city = cities[cityid];
    coordinate c = map.to_screen(city->latitude, city->longitude);

    city->resources[COINS] = 120;
    drawCityScreen(c.lat, c.lon, city);
    if (drawn(NOGOLD))
    {
        message = std::string("a city with 120 coins in its treasure drew nogold.png.");
        return 0;
    }
    // @NOTE: gold.png can't be checked this way: other boxes on this screen load it too.

    city->resources[COINS] = -15;
    drawCityScreen(c.lat, c.lon, city);
    if (!drawn(NOGOLD))
    {
        message = std::string("a city 15 coins in debt did not draw nogold.png in its treasure.");
        return 0;
    }

    haspassed = true;

    return 0;
}
std::string TestCase_086::title()
{
    return std::string("Treasure box: city->resources[COINS] as gold.png icons (nogold.png in debt), all inside the box.");

}

bool TestCase_086::done()
{
    return isdone;
}
bool TestCase_086::passed()
{
    return haspassed;
}
std::string TestCase_086::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_086();
}
