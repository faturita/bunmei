//  TestCase_027.cpp
//  bunmei
//
//  Created by Claude on 19/08/2026
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

#include "testcase_027.h"

// New resource-gated improvements (task #11): Quarry/Camp/Derrick/Plantation follow the
// exact same multi-turn mechanism as Road/Mine/Irrigation/Railroad (testcase_022/023) --
// BuildQuarryOrder etc. put the worker into a working state (Unit::quarrying(effort), effort
// looked up in improvementeffort) and processWork() spends the worker's turn on the task
// until a BuildQuarry finalize command flips the tile's improvement bit -- but they ALSO
// require the tile to carry the matching special resource (tileHasRequiredResource(),
// improvementresources table in tiles.cpp, per README.md's resource table): the order is
// silently rejected (a message() is logged, the worker keeps its moves) when the resource
// is missing.  This test drives BuildQuarryOrder on a tile with NO marble (must be rejected)
// and then on a tile WITH marble (must take several turns, same as testcase_022), and
// separately sanity-checks the improvementresources/improvementeffort tables for the other
// three new types (Camp/Derrick/Plantation) directly.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Tiles tiles;
extern MovementCost movementcosts;
extern ImprovementEffort improvementeffort;
extern ImprovementResources improvementresources;
extern std::unordered_map<int, Improvement*> improvements;
extern bool autoEndOfTurn;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_027::TestCase_027()
{

}

TestCase_027::~TestCase_027()
{

}

int TestCase_027::number()
{
    return 27;
}

void TestCase_027::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initMovementCosts(movementcosts);
    initImprovementEffort(improvementeffort);
    initImprovementResources(improvementresources);
    initImprovements(improvements);     // The quarry sprite: drawMap dereferences improvements[] entries.

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

    // (0,0) has no special resource: a quarry must NOT be buildable there.
    map.set(0,0).bioma = GRASSLAND;

    // (1,0) has MARBLE: a quarry must be buildable there, taking several turns
    // (effort 9 for GRASSLAND, see initImprovementEffort).
    map.set(1,0).bioma = GRASSLAND;
    map.set(1,0).resource = MARBLE;

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

int TestCase_027::check(int year)
{

    ticks++;

    Unit *u = units[workerid];

    // The improvementresources/improvementeffort tables for the three other new types
    // (Camp/Derrick/Plantation) are checked directly, once, rather than driving a full
    // multi-turn simulation for each (Quarry below already exercises the mechanism itself).
    if (ticks == 1)
    {
        if (!tileHasRequiredResource(improvementresources, CAMP, DOE) ||
            !tileHasRequiredResource(improvementresources, CAMP, GAME) ||
            !tileHasRequiredResource(improvementresources, CAMP, SEAL) ||
            tileHasRequiredResource(improvementresources, CAMP, MARBLE))
        {
            isdone = true;
            haspassed = false;
            message = std::string("Camp resource requirements (DOE/GAME/SEAL) are wrong.");
            return 0;
        }

        if (!tileHasRequiredResource(improvementresources, DERRICK, OIL) ||
            tileHasRequiredResource(improvementresources, DERRICK, MARBLE))
        {
            isdone = true;
            haspassed = false;
            message = std::string("Derrick resource requirement (OIL) is wrong.");
            return 0;
        }

        if (!tileHasRequiredResource(improvementresources, PLANTATION, GRAPES) ||
            !tileHasRequiredResource(improvementresources, PLANTATION, SUGAR) ||
            !tileHasRequiredResource(improvementresources, PLANTATION, TOBACCO) ||
            !tileHasRequiredResource(improvementresources, PLANTATION, COTTON) ||
            tileHasRequiredResource(improvementresources, PLANTATION, OIL))
        {
            isdone = true;
            haspassed = false;
            message = std::string("Plantation resource requirements (GRAPES/SUGAR/TOBACCO/COTTON) are wrong.");
            return 0;
        }

        if (getImprovementEffort(improvementeffort, CAMP, GRASSLAND) <= 0 ||
            getImprovementEffort(improvementeffort, DERRICK, GRASSLAND) <= 0 ||
            getImprovementEffort(improvementeffort, PLANTATION, GRASSLAND) <= 0)
        {
            isdone = true;
            haspassed = false;
            message = std::string("Camp/Derrick/Plantation have no effort configured for GRASSLAND.");
            return 0;
        }
    }

    // Give the BuildQuarryOrder while standing on (0,0), which has no marble: the order
    // must be silently rejected.
    if (ticks == 300)
    {
        CommandOrder co;
        co.command = Command::BuildQuarryOrder;
        co.parameters.spawnid = workerid;
        co.parameters.factionid = u->faction;
        coordinator.push(co);
    }

    if (ticks == 303)
    {
        if (u->isQuarrying() || map.set(0,0).hasQuarry())
        {
            isdone = true;
            haspassed = false;
            message = std::string("BuildQuarryOrder was accepted on a tile with no marble.");
            return 0;
        }
    }

    // Move the worker onto (1,0), which HAS marble, and try again: this time the order
    // must be accepted and the worker must enter the 'quarrying' state instead of building
    // instantly.
    if (ticks == 304)
    {
        u->latitude = 1;
        u->longitude = 0;
        u->availablemoves = u->getUnitMoves();

        CommandOrder co;
        co.command = Command::BuildQuarryOrder;
        co.parameters.spawnid = workerid;
        co.parameters.factionid = u->faction;
        coordinator.push(co);
    }

    if (ticks == 307)
    {
        if (map.set(1,0).hasQuarry())
        {
            isdone = true;
            haspassed = false;
            message = std::string("Quarry was built instantly; BuildQuarryOrder must take several turns (see improvementeffort).");
            return 0;
        }
        if (!u->isQuarrying())
        {
            isdone = true;
            haspassed = false;
            message = std::string("Worker is not in the 'quarrying' state shortly after a valid BuildQuarryOrder.");
            return 0;
        }
    }

    // Enough turns have now passed (effort=9 for GRASSLAND): the quarry must be built and
    // the worker must have left the 'quarrying' state.
    if (ticks == 330)
    {
        isdone = true;

        if (!map.set(1,0).hasQuarry())
        {
            haspassed = false;
            message = std::string("Quarry was never built after enough turns elapsed.");
            return 0;
        }

        haspassed = !u->isQuarrying();
        if (!haspassed)
            message = std::string("Worker is still 'quarrying' even though the quarry was completed.");
    }

    return 0;
}
std::string TestCase_027::title()
{
    return std::string("Resource-gated improvements: Quarry/Camp/Derrick/Plantation (BuildQuarryOrder etc. require the matching special resource).");

}

bool TestCase_027::done()
{
    return isdone;
}
bool TestCase_027::passed()
{
    return haspassed;
}
std::string TestCase_027::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_027();
}
