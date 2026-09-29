//  TestCase_077.cpp
//  bunmei
//
//  Created by faturita on 27/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../map.h"
#include "../units/Unit.h"
#include "../units/Warrior.h"
#include "../units/Settler.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../commandorder.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../cityscreenui.h"

#include "testcase_077.h"

// Command::JoinCityOrder ('J'): a unit joins back the city it stands on and is disbanded the
// same way a Settler is when it founds one. Its headcount goes into the city's food storage
// (City::increaseHeadCount) and every thresshold it fills becomes a population point, so the
// city's headcount grows by exactly the unit's. Driven through the real key, handleKeypress('J'),
// and processCommandOrders(). Refused: a unit on no city, a unit in another faction's city, and
// an order from a faction that does not own the unit.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern Factions factions;
extern Tiles tiles;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_077::TestCase_077()
{

}

TestCase_077::~TestCase_077()
{

}

int TestCase_077::number()
{
    return 77;
}

void TestCase_077::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }

    for (int lat=-12;lat<=12;lat++)
        for (int lon=-12;lon<=12;lon++)
            map.set(lat,lon) = mapcell(LAND);

    assignProductionRates(map);     // tiles yield food, so a grown city has tiles to pick

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon).setVisible(0);

    for (int f=0;f<2;f++)
    {
        Faction *faction = new Faction();
        faction->id = f;
        strcpy(faction->name, f == 0 ? "Vikings" : "Romans");
        faction->red = 255; faction->green = 0; faction->blue = 0;
        faction->autoPlayer = false;
        factions.push_back(faction);
        citynames[f] = std::queue<std::string>();
    }

    City *city = new City(&map, 0, getNextCityId(), 0, 0);
    city->setName("Kattegat");
    city->foundedyear = -4000;
    cities[city->id] = city;
    cityid = city->id;
    city->setCityPop(2);
    city->resources[FOOD] = 150;
    city->assignWorkingTile();       // allowance pop+1 = 3, full

    City *foreign = new City(&map, 1, getNextCityId(), 8, 8);
    foreign->setName("Roma");
    foreign->foundedyear = -4000;
    cities[foreign->id] = foreign;
    foreigncityid = foreign->id;

    auto place = [&](Unit* u, int faction, int lat, int lon)
    {
        u->id = getNextUnitId();
        u->faction = faction;
        u->latitude = lat; u->longitude = lon;
        u->availablemoves = u->getUnitMoves();
        units[u->id] = u;
        map.set(lat,lon).setOwnedBy(faction);
        return u->id;
    };

    warriorid  = place(new Warrior(), 0, 0, 0);     // in its own city
    settlerid  = place(new Settler(), 0, 0, 0);     // in its own city
    strayid    = place(new Warrior(), 0, 3, -3);    // on no city
    foreignerid= place(new Warrior(), 0, 8, 8);     // inside Roma

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

}

int TestCase_077::check(int year)
{

    ticks++;

    if (isdone)
        return 0;

    if (ticks < 3)
        return 0;

    City* city = cities[cityid];
    char buf[256];

    isdone = true;
    haspassed = false;

    auto pressJoin = [&](int unitid)
    {
        coordinator.a_f_id = 0;
        coordinator.a_u_id = unitid;
        handleKeypress('J', 0, 0);
        processCommandOrders();
    };

    int tilesBefore = city->numberOfWorkingTiles();
    if (tilesBefore != city->workingTileAllowance())
    {
        sprintf(buf,"setup: Kattegat works %d tiles, expected its full allowance %d.", tilesBefore, city->workingTileAllowance());
        message = std::string(buf);
        return 0;
    }

    // A Warrior (100) joins a pop 2 city holding 150 food: 250 fills pop 2's thresshold (200),
    // so the city grows to pop 3 with 50 food left, and works one more tile.
    {
        int hcBefore = city->getHeadCount();
        pressJoin(warriorid);
        if (units.find(warriorid) != units.end())
        {
            message = std::string("'J' on a Warrior in its own city did not disband it.");
            return 0;
        }
        if (city->getCityPop() != 3 || city->resources[FOOD] != 50 || city->getHeadCount() != hcBefore + 100)
        {
            sprintf(buf,"Warrior joined: pop %d food %d hc %d, expected pop 3 food 50 hc %d.",
                    city->getCityPop(), city->resources[FOOD], city->getHeadCount(), hcBefore + 100);
            message = std::string(buf);
            return 0;
        }
        if (city->numberOfWorkingTiles() != city->workingTileAllowance())
        {
            sprintf(buf,"after growing to pop %d the city works %d tiles, expected %d.",
                    city->getCityPop(), city->numberOfWorkingTiles(), city->workingTileAllowance());
            message = std::string(buf);
            return 0;
        }
        if (!map.set(0,0).isOwnedBy(0))
        {
            message = std::string("the city tile lost its faction ownership when the unit was released from it.");
            return 0;
        }
    }

    // A Settler joins with its own headcount (500): 50+500 crosses pop 3's thresshold (300)
    // once, leaving pop 4 with 250 food.
    {
        int hcBefore = city->getHeadCount();
        int settlerhc = units[settlerid]->getHeadCount();
        pressJoin(settlerid);
        if (units.find(settlerid) != units.end() || settlerhc != 500
            || city->getCityPop() != 4 || city->resources[FOOD] != 250 || city->getHeadCount() != hcBefore + 500)
        {
            sprintf(buf,"Settler (hc %d) joined: pop %d food %d hc %d, expected hc 500, pop 4 food 250 hc %d and the Settler gone.",
                    settlerhc, city->getCityPop(), city->resources[FOOD], city->getHeadCount(), hcBefore + 500);
            message = std::string(buf);
            return 0;
        }
        if (!map.set(0,0).isOwnedBy(0))
        {
            message = std::string("the city tile lost its faction ownership after the last unit joined.");
            return 0;
        }
    }

    // Refusals: nothing changes, the unit stays.
    int hcNow = city->getHeadCount();
    int romaHc = cities[foreigncityid]->getHeadCount();

    pressJoin(strayid);
    if (units.find(strayid) == units.end())
    {
        message = std::string("a Warrior standing on no city was disbanded by 'J'.");
        return 0;
    }

    pressJoin(foreignerid);
    if (units.find(foreignerid) == units.end() || cities[foreigncityid]->getHeadCount() != romaHc)
    {
        message = std::string("a Viking Warrior joined the Roman city.");
        return 0;
    }

    // An order for a Viking unit coming from the Romans: moved onto Kattegat first so the
    // city check alone would let it through.
    units[strayid]->latitude = 0; units[strayid]->longitude = 0;
    CommandOrder co;
    co.command = Command::JoinCityOrder;
    co.parameters.spawnid = strayid;
    co.parameters.factionid = 1;
    coordinator.push(co);
    processCommandOrders();
    if (units.find(strayid) == units.end() || city->getHeadCount() != hcNow)
    {
        message = std::string("the Romans made a Viking Warrior join Kattegat.");
        return 0;
    }

    haspassed = true;

    return 0;
}
std::string TestCase_077::title()
{
    return std::string("JoinCityOrder ('J'): a unit joins its own city, its headcount becomes food and pop, and it is disbanded.");

}

bool TestCase_077::done()
{
    return isdone;
}
bool TestCase_077::passed()
{
    return haspassed;
}
std::string TestCase_077::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_077();
}
