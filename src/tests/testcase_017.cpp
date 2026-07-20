//  TestCase_017.cpp
//  bunmei
//
//  Created by Claude on 16/07/2026
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
#include "../ai.h"

#include "testcase_017.h"

// Dijkstra pathfinding uses the terrain movement costs (initMovementCosts) as edge
// weights, adjusted by roads (ROAD_MOVEMENT_COST) and railroads (RAILROAD_MOVEMENT_COST).
// A warrior at (0,0) paths to (0,2) with a MOUNTAINS tile (cost 3) at (0,1):
//   - without roads the path goes AROUND the mountain (grass 1+1 < mountain 3+1),
//   - with a road over the mountain the path goes THROUGH it (1/3+1/3),
//   - with a railroad detour the path prefers the railroad (1/9+1/9 < 1/3+1/3).

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;
extern Tiles tiles;
extern std::unordered_map<int, Improvement*> improvements;
extern MovementCost movementcosts;

extern float mapzoom;

extern Coordinator coordinator;

#define TEST_MAPSIZE 1

TestCase_017::TestCase_017()
{

}

TestCase_017::~TestCase_017()
{

}

int TestCase_017::number()
{
    return 17;
}

void TestCase_017::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initImprovements(improvements);     // The road/railroad sprites: drawMap dereferences improvements[] entries.
    initMovementCosts(movementcosts);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }

    // A grassland island around the map center, with one mountain east of the warrior.
    for (int lat=-8;lat<=8;lat++)
    {
        for (int lon=-8;lon<=8;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
            map.set(lat,lon).bioma = GRASSLAND;
        }
    }

    map.set(0,1).bioma = MOUNTAINS;

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
    faction->autoPlayer = false;

    factions.push_back(faction);

    {
        coordinate c(0,0);

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

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = 1;

}

int TestCase_017::check(int year)
{

    ticks++;

    if (ticks == 300)
    {
        Unit *u = units[1];
        bool ok = false;

        // Case A: no roads.  Around the mountain (1+1) beats through it (3+1):
        // the first step must NOT be the mountain (0,1).
        coordinate c = goTo(u, ok, 0, 2);
        if (!ok || (c.lat == 0 && c.lon == 1))
        {
            isdone = true;
            haspassed = false;
            char msg[256];
            sprintf(msg,"Terrain cost: first step is (%d,%d) (ok=%d); the path should avoid the mountain (0,1).",c.lat,c.lon,ok);
            message = std::string(msg);
            return 0;
        }

        // Case B: a road through the mountain (1/3+1/3) beats going around (1+1):
        // the first step must now be the mountain (0,1).
        map.set(0,0).buildRoad();
        map.set(0,1).buildRoad();
        map.set(0,2).buildRoad();

        c = goTo(u, ok, 0, 2);
        if (!ok || !(c.lat == 0 && c.lon == 1))
        {
            isdone = true;
            haspassed = false;
            char msg[256];
            sprintf(msg,"Road cost: first step is (%d,%d) (ok=%d); the path should take the road over the mountain (0,1).",c.lat,c.lon,ok);
            message = std::string(msg);
            return 0;
        }

        // Case C: a railroad detour (1/9+1/9) beats the road over the mountain (1/3+1/3):
        // the first step must be the railroad tile (1,1).
        map.set(0,0).buildRailroad();
        map.set(1,1).buildRailroad();
        map.set(0,2).buildRailroad();

        c = goTo(u, ok, 0, 2);
        isdone = true;
        haspassed = (ok && c.lat == 1 && c.lon == 1);
        if (!haspassed)
        {
            char msg[256];
            sprintf(msg,"Railroad cost: first step is (%d,%d) (ok=%d); the path should take the railroad through (1,1).",c.lat,c.lon,ok);
            message = std::string(msg);
        }
    }

    return 0;
}
std::string TestCase_017::title()
{
    return std::string("Pathfinding weighs terrain movement costs, roads (1/3) and railroads (1/9).");

}

bool TestCase_017::done()
{
    return isdone;
}
bool TestCase_017::passed()
{
    return haspassed;
}
std::string TestCase_017::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_017();
}
