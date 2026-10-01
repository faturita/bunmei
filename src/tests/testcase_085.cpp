//  TestCase_085.cpp
//  bunmei
//
//  Created by faturita on 30/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../map.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../cityscreenui.h"
#include "../buildings/Barracks.h"

#include "testcase_085.h"

// The City Resources box flags a shortage: when a city's food or coins production this turn
// (tiles + converted trade) does not cover what it consumes (2 food per pop; 1 coin per
// building), the uncovered units are drawn with nofood.png / nogold.png.
//
// A rendered screen can't be inspected, but every icon is loaded into `maptextures` the first
// time it is drawn: so the real drawCityScreen() is rendered once with no shortage (neither
// icon may be loaded) and once with both shortages (both must be).

extern Map map;
extern std::unordered_map<int, City*> cities;
extern Factions factions;
extern Tiles tiles;
extern std::unordered_map<std::string, GLuint> maptextures;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

#define NOFOOD "assets/assets/city/nofood.png"
#define NOGOLD "assets/assets/city/nogold.png"

TestCase_085::TestCase_085()
{

}

TestCase_085::~TestCase_085()
{

}

int TestCase_085::number()
{
    return 85;
}

// The balance the City Resources box draws: production (tiles + converted trade) - consumption.
static int balance(City* city, int r)
{
    int production  = city->getProductionRate(r) + cityTradeConversionRate(city, r);
    int consumption = city->getConsumptionRate(r) + city->getBuildingConsumptionRate(r);
    return production - consumption;
}

static bool drawn(const char* icon)
{
    return maptextures.find(std::string(icon)) != maptextures.end();
}

void TestCase_085::init()
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
    city->assignWorkingTile();      // two grassland tiles feed the one pop
    city->assignWorkingTile();
    city->foundedyear = -4000;
    cities[city->id] = city;
    cityid = city->id;

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

}

int TestCase_085::check(int year)
{

    ticks++;

    if (isdone)
        return 0;

    if (ticks < 3)
        return 0;

    isdone = true;
    haspassed = false;
    char buf[300];

    City *city = cities[cityid];
    coordinate c = map.to_screen(city->latitude, city->longitude);

    // ---- no shortage: neither icon is drawn ---------------------------------------------------
    if (balance(city, FOOD) < 0 || balance(city, COINS) < 0)
    {
        snprintf(buf, sizeof(buf), "setup: the pop 1 city without buildings already has a shortage (food %d, coins %d).",
                 balance(city, FOOD), balance(city, COINS));
        message = std::string(buf);
        return 0;
    }
    if (drawn(NOFOOD) || drawn(NOGOLD))
    {
        message = std::string("setup: a shortage icon was drawn before this test rendered anything.");
        return 0;
    }
    drawCityScreen(c.lat, c.lon, city);
    if (drawn(NOFOOD) || drawn(NOGOLD))
    {
        message = std::string("a shortage icon was drawn for a city with enough food and coins.");
        return 0;
    }

    // ---- food and coin shortage: both icons are drawn --------------------------------------
    // Ten mouths on the same worked tiles, and three buildings at 1 coin each with no coin income.
    city->setCityPop(10);
    for (int i = 0; i < 3; i++)
        city->buildings.push_back(new Barracks());
    if (balance(city, FOOD) >= 0 || balance(city, COINS) >= 0)
    {
        snprintf(buf, sizeof(buf), "setup: expected a food and a coin shortage (food %d, coins %d).",
                 balance(city, FOOD), balance(city, COINS));
        message = std::string(buf);
        return 0;
    }
    drawCityScreen(c.lat, c.lon, city);
    if (!drawn(NOFOOD))
    {
        snprintf(buf, sizeof(buf), "food short by %d, but %s was not drawn.", -balance(city, FOOD), NOFOOD);
        message = std::string(buf);
        return 0;
    }
    if (!drawn(NOGOLD))
    {
        snprintf(buf, sizeof(buf), "coins short by %d, but %s was not drawn.", -balance(city, COINS), NOGOLD);
        message = std::string(buf);
        return 0;
    }

    haspassed = true;

    return 0;
}
std::string TestCase_085::title()
{
    return std::string("City Resources box: nofood/nogold icons drawn only when food or coins production does not cover consumption.");

}

bool TestCase_085::done()
{
    return isdone;
}
bool TestCase_085::passed()
{
    return haspassed;
}
std::string TestCase_085::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_085();
}
