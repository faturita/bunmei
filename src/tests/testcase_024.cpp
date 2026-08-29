//  TestCase_024.cpp
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

#include "testcase_024.h"

// Clicking on a working unit (task #6 follow-up on testcase_022/023) must interrupt the
// improvement in progress, exactly like clicking a fortified/sentried unit wakes it up:
// processMouse's unit-selection loop (usercontrols.cpp) now also calls Unit::completed()
// when the clicked unit isRoading/isMining/isIrrigating/isRailroading, clearing the working
// state WITHOUT applying the finalize command.  Since Unit::roading()/etc. always overwrite
// reqEffort unconditionally, a later order on the same worker starts over from scratch, not
// resuming leftover progress.  This test gives a worker a BuildRoadOrder (effort 6 on
// GRASSLAND, worker moves=2 so work() -- which now subtracts availablemoves per turn, not a
// flat 1 -- needs 3 turns: 6->4->2->0) and: (a) lets it accumulate PARTIAL progress (2 of the
// 3 turns) then simulates a click on its tile, checking the click selects the worker AND
// clears isRoading(); (b) lets many more turns pass with NO further order and confirms the
// road is NEVER built (the interrupted work must not silently keep completing in the
// background); (c) re-issues BuildRoadOrder and confirms it still completes correctly,
// proving the interrupt does not permanently wedge the worker.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Tiles tiles;
extern MovementCost movementcosts;
extern ImprovementEffort improvementeffort;
extern bool autoEndOfTurn;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

extern int REAL_SCREEN_WIDTH;
extern int REAL_SCREEN_HEIGHT;

#define TEST_MAPSIZE 1

TestCase_024::TestCase_024()
{

}

TestCase_024::~TestCase_024()
{

}

int TestCase_024::number()
{
    return 24;
}

void TestCase_024::init()
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

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            for(auto &r:ALL_CORE_RESOURCES)
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

int TestCase_024::check(int year)
{

    ticks++;

    Unit *u = units[workerid];

    // Give the BuildRoadOrder: the worker enters the "roading" state.
    if (ticks == 300)
    {
        CommandOrder co;
        co.command = Command::BuildRoadOrder;
        co.parameters.spawnid = workerid;
        co.parameters.factionid = u->faction;
        coordinator.push(co);
    }

    // Partial progress: 2 of the 3 required turns have run (6 -> 4 -> 2), not yet complete.
    // Interrupt it here with a simulated click on the worker's tile.
    if (ticks == 303)
    {
        if (map.set(0,0).hasRoad())
        {
            isdone = true;
            haspassed = false;
            message = std::string("Road was already built before the interrupting click; nothing left to interrupt.");
            return 0;
        }
        if (!u->isRoading())
        {
            isdone = true;
            haspassed = false;
            message = std::string("Worker is not 'roading' before the interrupting click.");
            return 0;
        }

        coordinate cs = map.to_screen(0,0);
        centermapinmap(cs.lat, cs.lon);
        processMouse(GLUT_LEFT_BUTTON, GLUT_DOWN, REAL_SCREEN_WIDTH/2, REAL_SCREEN_HEIGHT/2);

        if (coordinator.a_u_id != workerid)
        {
            isdone = true;
            haspassed = false;
            message = std::string("Clicking the working worker did not select it as the active unit.");
            return 0;
        }
        if (u->isRoading())
        {
            isdone = true;
            haspassed = false;
            message = std::string("Clicking the working worker did not interrupt its 'roading' state.");
            return 0;
        }
    }

    // A few more turns pass with no new order: the road must NEVER appear (the interrupted
    // work must not silently keep completing in the background from its old progress).
    // (The window ends before the re-issued order below is expected to complete.)
    if (ticks > 303 && ticks < 310 && map.set(0,0).hasRoad())
    {
        isdone = true;
        haspassed = false;
        message = std::string("Road got built after the interrupt even though no new order was given.");
        return 0;
    }

    // Re-issue the order: it must still work, completing from a fresh effort (3 more turns).
    if (ticks == 310)
    {
        CommandOrder co;
        co.command = Command::BuildRoadOrder;
        co.parameters.spawnid = workerid;
        co.parameters.factionid = u->faction;
        coordinator.push(co);
    }

    if (ticks == 330)
    {
        isdone = true;

        if (!map.set(0,0).hasRoad())
        {
            haspassed = false;
            message = std::string("Road was never built by the second BuildRoadOrder after the interrupt.");
            return 0;
        }

        haspassed = !u->isRoading();
        if (!haspassed)
            message = std::string("Worker is still 'roading' even though the second attempt completed.");
    }

    return 0;
}
std::string TestCase_024::title()
{
    return std::string("Clicking a working unit interrupts its improvement in progress and resets its effort.");

}

bool TestCase_024::done()
{
    return isdone;
}
bool TestCase_024::passed()
{
    return haspassed;
}
std::string TestCase_024::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_024();
}
