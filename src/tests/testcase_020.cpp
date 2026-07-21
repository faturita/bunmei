//  TestCase_020.cpp
//  bunmei
//
//  Created by Claude on 21/07/2026
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
#include "../map.h"
#include "../coordinator.h"
#include "../units/Settler.h"
#include "../engine.h"
#include "../tiles.h"

#include "testcase_020.h"

// Faction id 3 (the Chinnese, activated by task #1's initFactions() table) crashed on its
// first BuildCityOrder: initNaming() (tiles.cpp) only ever filled citynames[0..2], so
// citynames[3] was a DEFAULT-CONSTRUCTED EMPTY queue and processCommandOrders' BuildCityOrder
// handler called .front() on it -- UB, reproduced live as a SIGSEGV in
// './bunmei -nointro -mapsize 2' shortly after the first end of year.  This drives a faction
// 3 settler through the exact same BuildCityOrder path and checks a real city comes out.

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
#define SETTLER_ID 1

TestCase_020::TestCase_020()
{

}

TestCase_020::~TestCase_020()
{

}

int TestCase_020::number()
{
    return 20;
}

void TestCase_020::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initNaming(citynames);         // Must fill citynames[3] (Chinnese) too -- the bug.

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }

    for (int lat=-8;lat<=8;lat++)
    {
        for (int lon=-8;lon<=8;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
        }
    }

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

    // `factions` is indexed by VECTOR POSITION, not by Faction::id, so all four slots must
    // exist for `factions[3]` (used throughout bunmei.cpp/ai.cpp as coordinator.a_f_id) to be
    // valid -- matching how the real game's initFactions() pushes all four in order.  Only
    // faction 3 (Chinnese, the id that was never given a name pool) is actually driven.
    for (int i=0;i<=2;i++)
    {
        Faction *filler = new Faction();
        filler->id = i;
        strcpy(filler->name,"Filler");
        filler->red = 128; filler->green = 128; filler->blue = 128;
        filler->autoPlayer = true;
        factions.push_back(filler);
    }

    Faction *faction = new Faction();
    faction->id = 3;
    strcpy(faction->name,"Chinnese");
    faction->red = 0;
    faction->green = 255;
    faction->blue = 255;
    faction->autoPlayer = false;

    factions.push_back(faction);

    {
        coordinate c(0,0);

        Settler *s = new Settler();
        s->longitude = c.lon;
        s->latitude = c.lat;
        s->id = getNextUnitId();               // SETTLER_ID
        s->faction = 3;
        s->availablemoves = s->getUnitMoves();

        units[s->id] = s;
        map.set(c.lat,c.lon).setOwnedBy(3);
    }

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 3;
    coordinator.a_u_id = SETTLER_ID;

}

int TestCase_020::check(int year)
{

    ticks++;

    // Issue the same order 'b' sends: BuildCityOrder for the active unit.
    if (ticks == 300)
    {
        CommandOrder co;
        co.command = Command::BuildCityOrder;
        coordinator.push(co);
    }

    if (ticks == 400)
    {
        isdone = true;

        if (cities.empty())
        {
            haspassed = false;
            message = std::string("No city was founded for faction 3 (Chinnese).");
            return 0;
        }

        City *city = cities.begin()->second;

        if (city->faction != 3)
        {
            haspassed = false;
            char msg[256];
            sprintf(msg,"City founded for faction %d, expected faction 3.",city->faction);
            message = std::string(msg);
            return 0;
        }

        if (strlen(city->name) == 0)
        {
            haspassed = false;
            message = std::string("City for faction 3 has an empty name: citynames[3] was not populated.");
            return 0;
        }

        haspassed = true;
    }

    return 0;
}
std::string TestCase_020::title()
{
    return std::string("Faction 3 (Chinnese) founds a city without crashing: citynames[3] must be populated.");

}

bool TestCase_020::done()
{
    return isdone;
}
bool TestCase_020::passed()
{
    return haspassed;
}
std::string TestCase_020::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_020();
}
