//  TestCase_011.cpp
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
#include "../engine.h"
#include "../tiles.h"

#include "testcase_011.h"

TestCase_011::TestCase_011()
{

}

TestCase_011::~TestCase_011()
{

}

int TestCase_011::number()
{
    return 11;
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

// A Roman settler is stranded ON land owned by a Viking city (this happens in real games:
// cities claim working tiles every year, so tiles flip owner under enemy units passing by).
// The settler must be able to walk out on its own and found a city somewhere else; before
// the fix ai.cpp goTo() refused to path from enemy-owned land, autoPlayer re-issued the
// same GoTo every tick, and the game froze with the turn never ending.
void TestCase_011::init()
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
                map.set(lat,lon).addResourceProductionRate(2);
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

    // A Viking city at (0,0): its tile is owned by faction 0.
    {
        City *city = new City(&map, 0, getNextCityId(), 0, 0);
        city->setName("Kattegate");
        city->foundedyear = -4000;
        cities[city->id] = city;
    }

    // A Roman settler standing ON the Viking city tile (do not touch the tile
    // ownership: it belongs to the Viking city, exactly as after the tile flipped).
    {
        Settler *s = new Settler();
        s->longitude = 0;
        s->latitude = 0;
        s->id = getNextUnitId();
        s->faction = 1;
        s->availablemoves = s->getUnitMoves();

        units[s->id] = s;
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

int TestCase_011::check(int year)
{

    ticks++;

    // The stranded settler escaped the enemy-owned tile and founded its own city.
    for (auto& [k,c] : cities)
    {
        if (c->faction == 1)
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
        message = "The settler stranded on enemy-owned land never escaped and founded a city.";
    }

    return 0;
}
std::string TestCase_011::title()
{
    return std::string("A settler stranded on enemy-owned land walks out on its own and founds a city.");

}

bool TestCase_011::done()
{
    return isdone;
}
bool TestCase_011::passed()
{
    return haspassed;
}
std::string TestCase_011::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_011();
}
