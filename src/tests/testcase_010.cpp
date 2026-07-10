//  TestCase_010.cpp
//  bunmei
//
//  Created by Claude on 09/07/2026
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
#include "../units/Archer.h"
#include "../engine.h"
#include "../tiles.h"

#include "testcase_010.h"

TestCase_010::TestCase_010()
{

}

TestCase_010::~TestCase_010()
{

}

int TestCase_010::number()
{
    return 10;
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

#define SETTLER_ID 1

// A Viking city holds only a settler: a settler (defense 0) cannot defend a city, so the
// city stays capturable, and when the Roman archer takes it the settler must be captured
// too and flip to the Roman faction.
void TestCase_010::init()
{

    map.init(MAPHALFHEIGHT,MAPHALFWIDTH);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }


    // One big landmass.
    for (int lat=-10;lat<=10;lat++)
    {
        for (int lon=-10;lon<=10;lon++)
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

    faction = new Faction();
    faction->id = 1;
    strcpy(faction->name,"Romans");
    faction->red = 255;
    faction->green = 255;
    faction->blue = 0;
    faction->autoPlayer = true;

    factions.push_back(faction);

    // A Viking city at (0,0).
    {
        City *city = new City(&map, 0, getNextCityId(), 0, 0);
        city->setName("Kattegate");
        city->foundedyear = -4000;
        cities[city->id] = city;
    }

    // A Viking settler inside its own city.  It is fortified so it stays there, and it
    // must NOT count as a defender (its defense is 0).
    {
        Settler *s = new Settler();
        s->longitude = 0;
        s->latitude = 0;
        s->id = getNextUnitId();            // SETTLER_ID
        s->faction = 0;
        s->availablemoves = s->getUnitMoves();
        s->fortify();

        units[s->id] = s;
    }

    // A Roman archer nearby: the autoplayer sends it to capture the undefended city.
    {
        Archer *archer = new Archer();
        archer->longitude = 5;
        archer->latitude = 5;
        archer->id = getNextUnitId();
        archer->faction = 1;
        archer->availablemoves = archer->getUnitMoves();

        units[archer->id] = archer;
        map.set(5,5).setOwnedBy(1);
    }

    citynames[0] = std::queue<std::string>();
    citynames[1] = std::queue<std::string>();

    for(int i=0;i<20;i++)
    {
        citynames[0].push("Jorvik");
        citynames[0].push("Hedeby");
        citynames[1].push("Roma");
        citynames[1].push("Neapolis");
        citynames[1].push("Pompeii");
    }

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = 1;

}

int TestCase_010::check(int year)
{

    ticks++;

    // The settler must never die in this scenario: it is captured, not fought.
    if (units.find(SETTLER_ID) == units.end())
    {
        isdone = true;
        haspassed = false;
        message = "The settler was destroyed instead of being captured with the city.";
        return 0;
    }

    // The city was captured and the settler flipped to the conquering faction.
    for (auto& [k,c] : cities)
    {
        if (c->faction == 1 && units[SETTLER_ID]->faction == 1)
        {
            isdone = true;
            haspassed = true;
            return 0;
        }
    }

    if (ticks > 20000)
    {
        isdone = true;
        haspassed = false;
        message = "The city was never captured together with its settler.";
    }

    return 0;
}
std::string TestCase_010::title()
{
    return std::string("A settler cannot defend a city: it is captured and flips to the conquering faction.");

}

bool TestCase_010::done()
{
    return isdone;
}
bool TestCase_010::passed()
{
    return haspassed;
}
std::string TestCase_010::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_010();
}
