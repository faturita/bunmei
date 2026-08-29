//  TestCase_022.cpp
//  bunmei
//
//  Created by Claude on 27/07/2026
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
#include "../units/Worker.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../map.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"

#include "testcase_022.h"

// Building an improvement (task #3 follow-up) now takes several turns instead of consuming
// a worker's whole turn and completing instantly: BuildRoadOrder puts the worker into the
// "roading" state (Unit::roading(effort), effort looked up in improvementeffort by the
// tile's bioma) and processWork() spends the worker's turn on the task, calling
// Unit::work(), until the effort reaches zero and a BuildRoad finalize command actually
// flips the tile's improvement bit.  This test gives a worker a BuildRoadOrder on a
// GRASSLAND tile (effort 6, see initImprovementEffort) and checks: (a) shortly after the
// order the tile is NOT yet a road and the worker is still "roading" (it takes more than
// one turn), and (b) after enough turns have passed the tile IS a road and the worker is
// no longer "roading".

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;
extern Tiles tiles;
extern MovementCost movementcosts;
extern ImprovementEffort improvementeffort;
extern bool autoEndOfTurn;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_022::TestCase_022()
{

}

TestCase_022::~TestCase_022()
{

}

int TestCase_022::number()
{
    return 22;
}

void TestCase_022::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initMovementCosts(movementcosts);
    initImprovementEffort(improvementeffort);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }

    // A small island around the map center.
    for (int lat=-8;lat<=8;lat++)
    {
        for (int lon=-8;lon<=8;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
        }
    }

    // GRASSLAND costs 6 worker-turns to road (initImprovementEffort default).
    map.set(0,0).bioma = GRASSLAND;

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

    // A single faction with a single unit: nothing else ever contests turn order, so
    // autoEndOfTurn (always true in the real game, see gamekernel.cpp) is set here too,
    // otherwise nothing would end the worker's turn once it becomes the last unit with
    // no moves left (the testcase build never sets it, unlike gamekernel.cpp).
    autoEndOfTurn = true;

    {
        coordinate c(0,0);

        Worker *w = new Worker();
        w->longitude = c.lon;
        w->latitude = c.lat;
        w->id = getNextUnitId();
        w->faction = 0;
        w->availablemoves = w->getUnitMoves();

        units[w->id] = w;
        workerid = w->id;
        map.set(c.lat,c.lon).setOwnedBy(0);
    }

    citynames[0] = std::queue<std::string>();
    for(int i=0;i<20;i++)
        citynames[0].push("Kattegate");

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = workerid;

}

int TestCase_022::check(int year)
{

    ticks++;

    Unit *u = units[workerid];

    // Give the BuildRoadOrder: the worker should enter the "roading" state instead of
    // building instantly.
    if (ticks == 300)
    {
        CommandOrder co;
        co.command = Command::BuildRoadOrder;
        co.parameters.spawnid = workerid;
        co.parameters.factionid = u->faction;
        coordinator.push(co);
    }

    // Shortly after the order: the road must NOT be built yet, and the worker must still
    // be roading (this is the whole point of the task: it takes more than one turn).
    if (ticks == 303)
    {
        if (map.set(0,0).hasRoad())
        {
            isdone = true;
            haspassed = false;
            message = std::string("Road was built instantly; BuildRoadOrder must take several turns (see improvementeffort).");
            return 0;
        }
        if (!u->isRoading())
        {
            isdone = true;
            haspassed = false;
            message = std::string("Worker is not in the 'roading' state shortly after BuildRoadOrder.");
            return 0;
        }
    }

    // Enough turns have now passed (effort=6 for GRASSLAND): the road must be built and
    // the worker must have left the 'roading' state.
    if (ticks == 320)
    {
        isdone = true;

        if (!map.set(0,0).hasRoad())
        {
            haspassed = false;
            message = std::string("Road was never built after enough turns elapsed.");
            return 0;
        }

        haspassed = !u->isRoading();
        if (!haspassed)
            message = std::string("Worker is still 'roading' even though the road was completed.");
    }

    return 0;
}
std::string TestCase_022::title()
{
    return std::string("Multi-turn worker improvements: BuildRoadOrder takes several turns via Unit::work().");

}

bool TestCase_022::done()
{
    return isdone;
}
bool TestCase_022::passed()
{
    return haspassed;
}
std::string TestCase_022::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_022();
}
