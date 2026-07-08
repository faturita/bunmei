//  TestCase_007.cpp
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

#include "testcase_007.h"

TestCase_007::TestCase_007()
{

}

TestCase_007::~TestCase_007()
{

}

int TestCase_007::number()
{
    return 7;
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

// The test runs on mapsize 3 (288x192), which covers the screen at mapzoom 0.25.
#define TEST_MAPSIZE 3

// The unit is placed far away from the map center.
#define UNIT_LAT 60
#define UNIT_LON 120

void TestCase_007::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

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

    // One zoom level before the map covers the screen: the camera must follow the unit.
    mapzoom = dimension.defaultzoom*2;
    centermapinmap(UNIT_LAT,UNIT_LON);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = 1;

}

int TestCase_007::check(int year)
{

    ticks++;

    // Verify the dimension table: every mapsize covers the whole screen at its
    // default zoom, and does NOT cover it one zoom level before (zoomed in).
    if (ticks == 1)
    {
        for(int ms=1;ms<=NUMBER_OF_MAPSIZES;ms++)
        {
            MapDimension d = getMapDimension(ms);

            bool covers    = (SCREEN_WIDTH/d.defaultzoom     >= d.halfwidth*2*16) && (SCREEN_HEIGHT/d.defaultzoom     >= d.halfheight*2*16);
            bool onebefore = (SCREEN_WIDTH/(d.defaultzoom*2) >= d.halfwidth*2*16) && (SCREEN_HEIGHT/(d.defaultzoom*2) >= d.halfheight*2*16);

            if (!covers || onebefore)
            {
                isdone = true;
                haspassed = false;
                char msg[256];
                sprintf(msg,"Mapsize %d (%dx%d) does not cover the screen exactly at mapzoom %f.",ms,d.halfwidth*2,d.halfheight*2,d.defaultzoom);
                message = std::string(msg);
                return 0;
            }
        }
    }

    // Phase 1: the map does not fit yet, the camera must remain centered on the unit.
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

        // Phase 2: zoom out to the default zoom of this mapsize.
        mapzoom = getMapDimension(TEST_MAPSIZE).defaultzoom;
    }

    // At the default zoom the whole map covers the screen, so the camera must lock to the map center.
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
std::string TestCase_007::title()
{
    return std::string("Map dimension presets cover the screen at their default zoom.");

}

bool TestCase_007::done()
{
    return isdone;
}
bool TestCase_007::passed()
{
    return haspassed;
}
std::string TestCase_007::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_007();
}
