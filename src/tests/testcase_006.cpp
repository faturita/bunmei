//  TestCase_006.cpp
//  bunmei
//
//  Created by Claude on 07/07/2026
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
#include "../units/Warrior.h"
#include "../engine.h"
#include "../tiles.h"

#include "testcase_006.h"

TestCase_006::TestCase_006()
{

}

TestCase_006::~TestCase_006()
{

}

int TestCase_006::number()
{
    return 6;
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

// The unit is placed far away from the map center, on coordinates that only
// exist on the enlarged 144x96 map.
#define UNIT_LAT 30
#define UNIT_LON 55

void TestCase_006::init()
{

    map.init(MAPHALFHEIGHT,MAPHALFWIDTH);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }

    // A small island far away from the map center.
    for (int lat=UNIT_LAT-3;lat<=UNIT_LAT+3;lat++)
    {
        for (int lon=UNIT_LON-3;lon<=UNIT_LON+3;lon++)
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
    faction->autoPlayer = false;

    factions.push_back(faction);

    {
        coordinate c(UNIT_LAT,UNIT_LON);

        Warrior *w = new Warrior();
        w->longitude = c.lon;
        w->latitude = c.lat;
        w->id = getNextUnitId();
        w->faction = 0;
        w->availablemoves = w->getUnitMoves();

        units[w->id] = w;
        map.set(c.lat,c.lon).setOwnedBy(0);
    }

    citynames[0] = std::queue<std::string>();

    for(int i=0;i<20;i++)
    {
        citynames[0].push("Kattegate");
        citynames[0].push("Jorvik");
        citynames[0].push("Hedeby");
    }

    // At mapzoom 1 the enlarged map does NOT fit in the viewport, so the camera
    // must stay centered wherever it was placed (on the unit).
    mapzoom = 1;
    centermapinmap(UNIT_LAT,UNIT_LON);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = 1;

}

int TestCase_006::check(int year)
{

    ticks++;

    // Phase 1: at mapzoom 1 the camera must remain centered on the unit.
    if (ticks == 300)
    {
        coordinate c = getCurrentCenter();
        if (!(c.lat == UNIT_LAT && c.lon == UNIT_LON))
        {
            isdone = true;
            haspassed = false;
            char msg[256];
            sprintf(msg,"Camera moved away from the unit: center is (%d,%d) instead of (%d,%d).",c.lat,c.lon,UNIT_LAT,UNIT_LON);
            message = std::string(msg);
            return 0;
        }

        // Phase 2: zoom out until the whole map fits in the viewport.
        mapzoom = 0.5;
    }

    // At mapzoom 0.5 the whole 144x96 map fits, so the camera must lock to the map center.
    if (ticks == 600)
    {
        coordinate c = getCurrentCenter();
        isdone = true;
        haspassed = (c.lat == 0 && c.lon == 0);
        if (!haspassed)
        {
            char msg[256];
            sprintf(msg,"Camera did not lock to the map center when the map fits: center is (%d,%d).",c.lat,c.lon);
            message = std::string(msg);
        }
    }

    return 0;
}
std::string TestCase_006::title()
{
    return std::string("Camera follows units on the enlarged map on every zoom level.");

}

bool TestCase_006::done()
{
    return isdone;
}
bool TestCase_006::passed()
{
    return haspassed;
}
std::string TestCase_006::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_006();
}
