//  TestCase_012.cpp
//  bunmei
//
//  Created by Claude on 13/07/2026
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

#include "testcase_012.h"

TestCase_012::TestCase_012()
{

}

TestCase_012::~TestCase_012()
{

}

int TestCase_012::number()
{
    return 12;
}

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;
extern Tiles tiles;

extern float mapzoom;

extern Coordinator coordinator;

static int settlerid = 0;

// An AI settler on a landmass with no good city spot left (the only land is a 7x7
// patch entirely within CITY_SPACING of an existing city).  Before the fix ai.cpp
// just set availablemoves=0 and nothing advanced coordinator.a_u_id: the settler
// blinked forever and the game waited for the user to press a movement key.  Now
// the AI must disband the settler (DisbandUnitOrder), which also switches to the
// next movable unit.
void TestCase_012::init()
{

    map.init(MAPHALFHEIGHT,MAPHALFWIDTH);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }


    // A small landmass: every land tile is within CITY_SPACING (3) of the city at (0,0),
    // so there is no good spot left for a second city.
    for (int lat=-3;lat<=3;lat++)
    {
        for (int lon=-3;lon<=3;lon++)
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
                map.set(lat,lon).resource_production_rate.push_back(2);
            }
        }

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255;
    faction->green = 0;
    faction->blue = 0;
    faction->autoPlayer = true;

    factions.push_back(faction);

    // The city that saturates the whole landmass.
    {
        City *city = new City(&map, 0, getNextCityId(), 0, 0);
        city->setName("Kattegate");
        city->foundedyear = -4000;
        cities[city->id] = city;
    }

    // A settler of the same faction with nowhere left to settle.
    {
        Settler *s = new Settler();
        s->longitude = 2;
        s->latitude = 2;
        s->id = getNextUnitId();
        s->faction = 0;
        s->availablemoves = s->getUnitMoves();

        units[s->id] = s;
        settlerid = s->id;
    }

    citynames[0] = std::queue<std::string>();

    for(int i=0;i<20;i++)
    {
        citynames[0].push("Jorvik");
        citynames[0].push("Hedeby");
    }

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = settlerid;

}

int TestCase_012::check(int year)
{

    ticks++;

    // The dead-ended settler was disbanded by the AI.
    if (units.find(settlerid) == units.end())
    {
        isdone = true;
        haspassed = true;
        return 0;
    }

    if (ticks > 5000)
    {
        isdone = true;
        haspassed = false;
        message = "The settler with no place left to settle was never disbanded (stuck blinking).";
    }

    return 0;
}
std::string TestCase_012::title()
{
    return std::string("An AI settler with no good city spot left on its landmass is disbanded.");

}

bool TestCase_012::done()
{
    return isdone;
}
bool TestCase_012::passed()
{
    return haspassed;
}
std::string TestCase_012::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_012();
}
