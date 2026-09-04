//  TestCase_049.cpp
//  bunmei
//
//  Created by Claude on 04/09/2026
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
#include "../marketui.h"

#include "testcase_049.h"

// @Task: Market screen (view=7, toggled '&'; marketui.cpp/.h) -- a drawInfoScreen-style
// full-screen table of every shippable resource, its price (prices[]) and the total amount
// held across every city the viewing faction can SEE (map cell isVisible, own or foreign).
// A rendered screen can't be inspected, so this exercises getShippableStockForFaction()
// directly, then does a sanity render.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Tiles tiles;
extern std::unordered_map<int, int> prices;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_049::TestCase_049() {}
TestCase_049::~TestCase_049() {}

int TestCase_049::number()
{
    return 49;
}

void TestCase_049::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initPrices(prices);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon) = mapcell(LAND);
    // Deliberately NOT setVisible() over the whole map -- visibility is set per city below.

    Faction *vikings = new Faction();
    vikings->id = 0; strcpy(vikings->name,"Vikings");
    vikings->red = 255; vikings->green = 0; vikings->blue = 0;
    vikings->autoPlayer = false;
    factions.push_back(vikings);

    Faction *mongols = new Faction();
    mongols->id = 1; strcpy(mongols->name,"Mongols");
    mongols->red = 0; mongols->green = 255; mongols->blue = 0;
    mongols->autoPlayer = false;
    factions.push_back(mongols);

    citynames[0] = std::queue<std::string>();
    citynames[1] = std::queue<std::string>();

    // cityA: Viking, VISIBLE to faction 0.
    City *cityA = new City(&map, 0, getNextCityId(), 2, 2);
    cityA->setName("Kattegate");
    cityA->commodities[copper] = 50;
    cityA->commodities[iron]   = 30;
    cityA->mfggoods[tools]     = 10;
    cities[cityA->id] = cityA;
    cityAid = cityA->id;

    // cityB: Viking, NOT visible to faction 0 -> its stock must be excluded.
    City *cityB = new City(&map, 0, getNextCityId(), -4, -4);
    cityB->setName("Uppsala");
    cityB->commodities[copper] = 100;
    cities[cityB->id] = cityB;
    cityBid = cityB->id;

    // cityC: Mongol (foreign), VISIBLE to faction 0 -> its stock IS included.
    City *cityC = new City(&map, 1, getNextCityId(), 6, 6);
    cityC->setName("Karakorum");
    cityC->commodities[copper] = 7;
    cityC->mfggoods[rum]       = 5;
    cities[cityC->id] = cityC;
    cityCid = cityC->id;

    map.set(2,2).setVisible(0);
    map.set(6,6).setVisible(0);
    // (-4,-4) left unexplored for faction 0.

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.v_f_id = 0;
}

int TestCase_049::check(int year)
{
    ticks++;
    if (isdone) return 0;

    // Same workaround as testcase_043: re-set the view every tick (tester resets it to 1).
    controller.view = 7;

    if (ticks < 3)
        return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    std::unordered_map<int,int> stock = getShippableStockForFaction(0);

    // Every commodity + mfg good id is present (seeded to 0).
    if ((int)stock.size() != (int)(sizeof(ALL_COMMODITIES)/sizeof(int) + sizeof(ALL_MFG_GOODS)/sizeof(int)))
    {
        fail("getShippableStockForFaction did not seed every shippable resource id.");
        return 0;
    }

    // copper: cityA (50, visible) + cityC (7, visible foreign); cityB (100) excluded (unseen).
    if (stock[copper] != 57)
    {
        char buf[128];
        snprintf(buf,sizeof(buf),"copper total was %d, expected 57 (cityA 50 + cityC 7; cityB 100 must be excluded).", stock[copper]);
        fail(buf);
        return 0;
    }
    if (stock[iron] != 30) { fail("iron total wrong (expected 30 from cityA)."); return 0; }
    if (stock[tools] != 10) { fail("tools total wrong (expected 10 from cityA)."); return 0; }
    if (stock[rum] != 5)   { fail("rum total wrong (expected 5 from the visible foreign cityC)."); return 0; }
    if (stock[silver] != 0 || stock[guns] != 0)
    {
        fail("a resource nobody stocks came back non-zero.");
        return 0;
    }

    // Faction 1 has explored none of these cities -> sees nothing.
    std::unordered_map<int,int> stock1 = getShippableStockForFaction(1);
    if (stock1[copper] != 0)
    {
        fail("getShippableStockForFaction(1) counted stock from cities faction 1 cannot see.");
        return 0;
    }

    if (prices[copper] != 1 || prices[robotics] != 1)
    {
        fail("prices[] not seeded to 1 (initPrices).");
        return 0;
    }

    // Sanity render with the screen populated: must not crash.
    drawMarketScreen();

    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_049::title()
{
    return std::string("Market screen (view=7): getShippableStockForFaction() sums shippable stock across the faction's VISIBLE cities only; drawMarketScreen() renders without crashing.");
}

bool TestCase_049::done()   { return isdone; }
bool TestCase_049::passed() { return haspassed; }
std::string TestCase_049::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_049();
}
