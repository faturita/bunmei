//  TestCase_016.cpp
//  bunmei
//
//  Created by Claude on 16/07/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

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
#include "../units/Archer.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"

#include "testcase_016.h"

// Terrain movement costs and movement debt: an archer (1 move per turn) ordered onto a
// MOUNTAINS tile (cost 3) does NOT move, its availablemoves goes to -2 (debt), it recovers
// one move per year and the pending move completes when the debt is paid: three turns to
// cross the mountain.  Roads override the terrain cost (ROAD_MOVEMENT_COST = 1/3), so the
// same archer then walks three road tiles in a single turn.

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
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_016::TestCase_016()
{

}

TestCase_016::~TestCase_016()
{

}

int TestCase_016::number()
{
    return 16;
}

void TestCase_016::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initImprovements(improvements);     // The road sprites: drawMap dereferences improvements[] entries.
    initMovementCosts(movementcosts);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }

    // An island around the map center.  The default bioma (0) costs 1 move per tile.
    for (int lat=-8;lat<=8;lat++)
    {
        for (int lon=-8;lon<=8;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
        }
    }

    // A mountain east of the warrior.
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

        Archer *w = new Archer();
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

int TestCase_016::check(int year)
{

    ticks++;

    Unit *u = units[1];

    // Phase 1: order the warrior east, onto the mountain.  The move goes into debt:
    // the unit stays, availablemoves = 1 - 3 = -2, the (only) unit has no moves left so
    // the turn ends and the endOfYear refresh brings the debt to -1.
    if (ticks == 300)
    {
        controller.registers.pitch = 0;
        controller.registers.roll = 1;
    }

    if (ticks == 400)
    {
        if (!(u->latitude == 0 && u->longitude == 0) || fabsf(u->availablemoves + 1.0f) > 0.01f)
        {
            isdone = true;
            haspassed = false;
            char msg[256];
            sprintf(msg,"Mountain move, first turn: unit at (%d,%d) with %.2f moves; expected still at (0,0) with -1 (debt).",u->latitude,u->longitude,u->availablemoves);
            message = std::string(msg);
            return 0;
        }
        coordinator.endofturn = true;
    }

    // Phase 2: second refresh pays the debt off (availablemoves 0) and the pending move
    // completes: the unit is now ON the mountain.  Three turns were spent on a cost-3 tile.
    if (ticks == 500)
    {
        if (!(u->latitude == 0 && u->longitude == 1) || fabsf(u->availablemoves) > 0.01f)
        {
            isdone = true;
            haspassed = false;
            char msg[256];
            sprintf(msg,"Mountain move, third turn: unit at (%d,%d) with %.2f moves; expected arrived at (0,1) with 0 moves.",u->latitude,u->longitude,u->availablemoves);
            message = std::string(msg);
            return 0;
        }
        coordinator.endofturn = true;
    }

    // Phase 3: the next year the unit has its move again.  Build a road under it and
    // eastwards: with ROAD_MOVEMENT_COST (1/3) the warrior walks three tiles in ONE turn.
    if (ticks == 600)
    {
        if (fabsf(u->availablemoves - 1.0f) > 0.01f)
        {
            isdone = true;
            haspassed = false;
            char msg[256];
            sprintf(msg,"After the mountain: %.2f moves; expected the full move back (1).",u->availablemoves);
            message = std::string(msg);
            return 0;
        }

        map.set(0,1).buildRoad();
        map.set(0,2).buildRoad();
        map.set(0,3).buildRoad();
        map.set(0,4).buildRoad();

        controller.registers.pitch = 0;
        controller.registers.roll = 1;
    }

    if (ticks == 700)
    {
        if (!(u->latitude == 0 && u->longitude == 2))
        {
            isdone = true;
            haspassed = false;
            char msg[256];
            sprintf(msg,"Road move 1: unit at (%d,%d); expected (0,2).",u->latitude,u->longitude);
            message = std::string(msg);
            return 0;
        }
        controller.registers.pitch = 0;
        controller.registers.roll = 1;
    }

    if (ticks == 800)
    {
        if (!(u->latitude == 0 && u->longitude == 3))
        {
            isdone = true;
            haspassed = false;
            char msg[256];
            sprintf(msg,"Road move 2: unit at (%d,%d); expected (0,3) on the same turn.",u->latitude,u->longitude);
            message = std::string(msg);
            return 0;
        }
        controller.registers.pitch = 0;
        controller.registers.roll = 1;
    }

    if (ticks == 900)
    {
        isdone = true;
        haspassed = (u->latitude == 0 && u->longitude == 4);
        if (!haspassed)
        {
            char msg[256];
            sprintf(msg,"Road move 3: unit at (%d,%d); expected (0,4): three road tiles in one turn.",u->latitude,u->longitude);
            message = std::string(msg);
        }
    }

    return 0;
}
std::string TestCase_016::title()
{
    return std::string("Movement debt: a mountain takes three turns to cross, roads take a third of a move.");

}

bool TestCase_016::done()
{
    return isdone;
}
bool TestCase_016::passed()
{
    return haspassed;
}
std::string TestCase_016::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_016();
}
