//  TestCase_038.cpp
//  bunmei
//
//  Created by Claude on 26/08/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../map.h"
#include "../units/Unit.h"
#include "../units/Worker.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"

#include "testcase_038.h"

// @Task: Irrigation placement rules. BuildIrrigationOrder (engine.cpp) is now gated by two
// independent checks, both enforced on the processCommand side (where the order is first
// received), same place the other resource-gated improvements (Quarry/Camp/Derrick/
// Plantation) already check tileHasRequiredResource:
//   1. Bioma deny-list (tileBiomaAllowsImprovement, tiles.h/.cpp): Irrigation cannot be built
//      on HILLS, ARCTIC or MOUNTAINS -- extends the exact allow/deny-table mechanism
//      ImprovementResources already uses, just inverted (a deny-list instead of an allow-list)
//      and keyed on the tile's BASE bioma (bioma & 0xf0) instead of its special resource.
//   2. Water/oasis/irrigation adjacency (tileHasWaterOasisOrIrrigationNearby, engine.cpp):
//      Irrigation must have a RIVER (any form -- the base RIVER bioma, or a RIVER_MOUTH_*
//      estuary variant), an OASIS special resource, a LAKE bioma tile, OR an ALREADY-IRRIGATED
//      tile as one of its 4 orthogonal (N/S/E/W) neighbours -- @Issue follow-up (issue.png):
//      irrigation forms a NETWORK that extends tile-by-tile away from a water source with no
//      distance limit (classic Civilization mechanic), not just tiles directly touching the
//      water/oasis/lake itself. LAKE is a new bioma (tiles.h) that gamekernel.cpp's
//      findOceanBodies() (BFS, same pattern as findLandmasses()) tags onto small enclosed
//      ocean bodies (landlocked oceans, <= LANDLOCKED_OCEAN_MAX_SIZE=30 tiles) at
//      map-generation time -- NOT exercised by this testcase since gamekernel.cpp/initMap()
//      is not linked into the testcase build (same gap noted in PROJECT.md for other
//      world-gen logic); this test instead places a LAKE-bioma tile directly, exactly what
//      that detection would have produced, and drives the real BuildIrrigationOrder handler
//      against it.
//   3. Already-built check (mapcell::hasIrrigation(), mapmodel.h -- @Issue follow-up):
//      BuildIrrigationOrder now also rejects a tile that already has irrigation, instead of
//      silently re-accepting (and re-running the several-turn build) on it.
//
// Six workers, six tiles, no water leaking between them (each row's OTHER three neighbours
// are always non-water/non-oasis/non-irrigated land or plain open ocean, which must NOT count):
//   (-4,0) GRASSLAND, west neighbour plain OCEAN (bioma 0)      -> REJECTED (no water nearby)
//   (-3,0) HILLS,     west neighbour LAND/RIVER                 -> REJECTED (bad bioma, even with water)
//   (-2,0) GRASSLAND, west neighbour LAND/RIVER                 -> ACCEPTED, full multi-turn cycle checked,
//                                                                   then a SECOND order on the same (now
//                                                                   irrigated) tile -> REJECTED
//   (-1,0) GRASSLAND, west neighbour LAND/GRASSLAND+OASIS       -> ACCEPTED
//   ( 0,0) GRASSLAND, west neighbour OCEAN/LAKE                 -> ACCEPTED
//   ( 1,0) GRASSLAND, west neighbour LAND/GRASSLAND+irrigated   -> ACCEPTED (irrigation chain,
//                                                                   the neighbour is NOT itself a
//                                                                   river/oasis/lake tile)

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;
extern Tiles tiles;
extern MovementCost movementcosts;
extern ImprovementEffort improvementeffort;
extern ImprovementResources improvementresources;
extern ImprovementBiomaRestrictions improvementbiomarestrictions;
extern std::unordered_map<int, Improvement*> improvements;
extern bool autoEndOfTurn;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

// Fortified so it is excluded from noMoreMovementsLeft()/nextMovableUnitId() (engine.cpp)
// until its own test phase explicitly wakes it (packUp()) -- with five coexisting workers,
// an idle one sitting on unused moves would otherwise block autoEndOfTurn/endOfYear forever
// (needed for the multi-turn completion check below) since ALL of a faction's units must be
// out of moves (or fortified/sentried) before a year can end.
static int spawnWorker(int lat, int lon)
{
    Worker *w = new Worker();
    w->longitude = lon;
    w->latitude = lat;
    w->id = getNextUnitId();
    w->faction = 0;
    w->availablemoves = w->getUnitMoves();
    w->fortify();

    units[w->id] = w;
    map.set(lat,lon).setOwnedBy(0);

    return w->id;
}

TestCase_038::TestCase_038()
{

}

TestCase_038::~TestCase_038()
{

}

int TestCase_038::number()
{
    return 38;
}

void TestCase_038::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initMovementCosts(movementcosts);
    initImprovementEffort(improvementeffort);
    initImprovementResources(improvementresources);
    initImprovementBiomaRestrictions(improvementbiomarestrictions);
    initImprovements(improvements);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }

    // Test row A (-4,0): no water/oasis/lake anywhere adjacent -- must be REJECTED.
    map.set(-4,0) = mapcell(LAND);
    map.set(-4,0).bioma = GRASSLAND;
    // West neighbour (-4,-1) stays plain OCEAN (bioma 0): a coastal ocean tile alone must NOT count.

    // Test row B (-3,0): HILLS with a RIVER neighbour -- must be REJECTED (bad bioma wins).
    map.set(-3,0) = mapcell(LAND);
    map.set(-3,0).bioma = HILLS;
    map.set(-3,-1) = mapcell(LAND);
    map.set(-3,-1).bioma = RIVER;

    // Test row C (-2,0): GRASSLAND with a RIVER neighbour -- must be ACCEPTED.
    map.set(-2,0) = mapcell(LAND);
    map.set(-2,0).bioma = GRASSLAND;
    map.set(-2,-1) = mapcell(LAND);
    map.set(-2,-1).bioma = RIVER;

    // Test row D (-1,0): GRASSLAND with an OASIS-resource neighbour -- must be ACCEPTED.
    map.set(-1,0) = mapcell(LAND);
    map.set(-1,0).bioma = GRASSLAND;
    map.set(-1,-1) = mapcell(LAND);
    map.set(-1,-1).bioma = GRASSLAND;
    map.set(-1,-1).resource = OASIS;

    // Test row E (0,0): GRASSLAND with a LAKE-bioma ocean neighbour -- must be ACCEPTED.
    map.set(0,0) = mapcell(LAND);
    map.set(0,0).bioma = GRASSLAND;
    map.set(0,-1) = mapcell(OCEAN);
    map.set(0,-1).bioma = LAKE;

    // Test row F (1,0): GRASSLAND next to an ALREADY-IRRIGATED tile that is otherwise plain
    // GRASSLAND (no river/oasis/lake of its own) -- must still be ACCEPTED: irrigation forms
    // a network that extends away from the original water source with no distance limit.
    map.set(1,0) = mapcell(LAND);
    map.set(1,0).bioma = GRASSLAND;
    map.set(1,-1) = mapcell(LAND);
    map.set(1,-1).bioma = GRASSLAND;
    map.set(1,-1).buildIrrigation();

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon).setVisible(0);

    resources.push_back(new Resource(0,0,"assets/assets/city/food.png","Food"));
    resources.push_back(new Resource(1,0,"assets/assets/city/production.png","Shields"));
    resources.push_back(new Resource(2,0,"assets/assets/city/trade.png","Trade"));
    resources.push_back(new Resource(3,0,"assets/assets/city/gold.png","Coins"));
    resources.push_back(new Resource(4,0,"assets/assets/city/bulb.png","Science"));
    resources.push_back(new Resource(5,0,"assets/assets/city/culture.png","Culture"));

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255;
    faction->green = 0;
    faction->blue = 0;
    faction->autoPlayer = false;

    factions.push_back(faction);

    // A single faction with several units: nothing else ever contests turn order, so
    // autoEndOfTurn (always true in the real game, see gamekernel.cpp) is set here too,
    // same as testcase_027.
    autoEndOfTurn = true;

    workerNoWaterId  = spawnWorker(-4,0);
    workerBadBiomaId = spawnWorker(-3,0);
    workerRiverId    = spawnWorker(-2,0);
    workerOasisId    = spawnWorker(-1,0);
    workerLakeId     = spawnWorker(0,0);
    workerChainId    = spawnWorker(1,0);

    citynames[0] = std::queue<std::string>();
    for(int i=0;i<20;i++)
        citynames[0].push("Kattegate");

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = workerRiverId;
}

int TestCase_038::check(int year)
{
    ticks++;

    // The bioma deny-list itself, checked directly (cheap, and pins down exactly which
    // biomas are restricted without needing a full simulated order for each).
    if (ticks == 1)
    {
        if (tileBiomaAllowsImprovement(improvementbiomarestrictions, IRRIGATION, HILLS) ||
            tileBiomaAllowsImprovement(improvementbiomarestrictions, IRRIGATION, ARCTIC) ||
            tileBiomaAllowsImprovement(improvementbiomarestrictions, IRRIGATION, MOUNTAINS))
        {
            isdone = true;
            haspassed = false;
            message = std::string("Irrigation must be denied on HILLS/ARCTIC/MOUNTAINS.");
            return 0;
        }

        if (!tileBiomaAllowsImprovement(improvementbiomarestrictions, IRRIGATION, GRASSLAND))
        {
            isdone = true;
            haspassed = false;
            message = std::string("Irrigation must be allowed on GRASSLAND.");
            return 0;
        }
    }

    // Row A: no water nearby -- must be rejected.
    if (ticks == 300)
    {
        Unit *u = units[workerNoWaterId];
        u->packUp();
        u->availablemoves = u->getUnitMoves();
        coordinator.a_u_id = workerNoWaterId;
        CommandOrder co;
        co.command = Command::BuildIrrigationOrder;
        co.parameters.spawnid = coordinator.a_u_id;
        co.parameters.factionid = coordinator.a_f_id;
        coordinator.push(co);
    }
    if (ticks == 303)
    {
        Unit *u = units[workerNoWaterId];
        if (u->isIrrigating() || (map.set(-4,0).improvements & IRRIGATION))
        {
            isdone = true;
            haspassed = false;
            message = std::string("BuildIrrigationOrder was accepted on a tile with no water/oasis/lake nearby.");
            return 0;
        }
        u->fortify();
    }

    // Row B: HILLS with a river neighbour -- must still be rejected (bioma wins).
    if (ticks == 304)
    {
        Unit *u = units[workerBadBiomaId];
        u->packUp();
        u->availablemoves = u->getUnitMoves();
        coordinator.a_u_id = workerBadBiomaId;
        CommandOrder co;
        co.command = Command::BuildIrrigationOrder;
        co.parameters.spawnid = coordinator.a_u_id;
        co.parameters.factionid = coordinator.a_f_id;
        coordinator.push(co);
    }
    if (ticks == 307)
    {
        Unit *u = units[workerBadBiomaId];
        if (u->isIrrigating() || (map.set(-3,0).improvements & IRRIGATION))
        {
            isdone = true;
            haspassed = false;
            message = std::string("BuildIrrigationOrder was accepted on HILLS even with a river neighbour.");
            return 0;
        }
        u->fortify();
    }

    // Row C: GRASSLAND with a river neighbour -- must be accepted, and take several turns
    // (same mechanism as Road/Mine/Quarry/etc, not built instantly). Only this worker is
    // awake (packUp()) during the wait below -- with every other worker fortified, ending a
    // turn/year (needed to refresh this worker's moves and let work() keep decrementing its
    // effort) only ever depends on THIS worker's moves reaching 0.
    if (ticks == 308)
    {
        Unit *u = units[workerRiverId];
        u->packUp();
        u->availablemoves = u->getUnitMoves();
        coordinator.a_u_id = workerRiverId;
        CommandOrder co;
        co.command = Command::BuildIrrigationOrder;
        co.parameters.spawnid = coordinator.a_u_id;
        co.parameters.factionid = coordinator.a_f_id;
        coordinator.push(co);
    }
    if (ticks == 311)
    {
        Unit *u = units[workerRiverId];
        if (map.set(-2,0).improvements & IRRIGATION)
        {
            isdone = true;
            haspassed = false;
            message = std::string("Irrigation was built instantly; BuildIrrigationOrder must take several turns.");
            return 0;
        }
        if (!u->isIrrigating())
        {
            isdone = true;
            haspassed = false;
            message = std::string("BuildIrrigationOrder was rejected on GRASSLAND with a river neighbour.");
            return 0;
        }
    }

    // Enough turns have now passed (effort=9 for GRASSLAND, see initImprovementEffort): the
    // irrigation must be built and the worker must have left the 'irrigating' state.
    if (ticks == 338)
    {
        Unit *u = units[workerRiverId];
        if (!(map.set(-2,0).improvements & IRRIGATION))
        {
            isdone = true;
            haspassed = false;
            message = std::string("Irrigation was never built after enough turns elapsed.");
            return 0;
        }
        if (u->isIrrigating())
        {
            isdone = true;
            haspassed = false;
            message = std::string("Worker is still 'irrigating' even though the irrigation was completed.");
            return 0;
        }
        u->fortify();
    }

    // Row C, again: the tile is already irrigated -- a second BuildIrrigationOrder on it
    // must be rejected, not re-accepted (@Issue follow-up to task #29).
    if (ticks == 339)
    {
        Unit *u = units[workerRiverId];
        u->packUp();
        u->availablemoves = u->getUnitMoves();
        coordinator.a_u_id = workerRiverId;
        CommandOrder co;
        co.command = Command::BuildIrrigationOrder;
        co.parameters.spawnid = coordinator.a_u_id;
        co.parameters.factionid = coordinator.a_f_id;
        coordinator.push(co);
    }
    if (ticks == 342)
    {
        Unit *u = units[workerRiverId];
        if (u->isIrrigating())
        {
            isdone = true;
            haspassed = false;
            message = std::string("BuildIrrigationOrder was accepted again on a tile that is already irrigated.");
            return 0;
        }
        u->fortify();
    }

    // Row D: GRASSLAND with an OASIS-resource neighbour -- must be accepted. Acceptance is
    // immediate (no need to wait for effort/turns), so no fortify/packUp juggling needed here.
    if (ticks == 343)
    {
        Unit *u = units[workerOasisId];
        u->packUp();
        u->availablemoves = u->getUnitMoves();
        coordinator.a_u_id = workerOasisId;
        CommandOrder co;
        co.command = Command::BuildIrrigationOrder;
        co.parameters.spawnid = coordinator.a_u_id;
        co.parameters.factionid = coordinator.a_f_id;
        coordinator.push(co);
    }
    if (ticks == 346)
    {
        Unit *u = units[workerOasisId];
        if (!u->isIrrigating())
        {
            isdone = true;
            haspassed = false;
            message = std::string("BuildIrrigationOrder was rejected on GRASSLAND with an OASIS neighbour.");
            return 0;
        }
        u->fortify();
    }

    // Row E: GRASSLAND with a LAKE-bioma neighbour -- must be accepted.
    if (ticks == 347)
    {
        Unit *u = units[workerLakeId];
        u->packUp();
        u->availablemoves = u->getUnitMoves();
        coordinator.a_u_id = workerLakeId;
        CommandOrder co;
        co.command = Command::BuildIrrigationOrder;
        co.parameters.spawnid = coordinator.a_u_id;
        co.parameters.factionid = coordinator.a_f_id;
        coordinator.push(co);
    }
    if (ticks == 350)
    {
        Unit *u = units[workerLakeId];
        if (!u->isIrrigating())
        {
            isdone = true;
            haspassed = false;
            message = std::string("BuildIrrigationOrder was rejected on GRASSLAND with a LAKE neighbour.");
            return 0;
        }
        u->fortify();
    }

    // Row F: GRASSLAND next to an already-irrigated (but otherwise plain GRASSLAND, not
    // itself river/oasis/lake) tile -- must be accepted: the irrigation network extends
    // tile-by-tile with no distance limit (@Issue follow-up, issue.png).
    if (ticks == 351)
    {
        Unit *u = units[workerChainId];
        u->packUp();
        u->availablemoves = u->getUnitMoves();
        coordinator.a_u_id = workerChainId;
        CommandOrder co;
        co.command = Command::BuildIrrigationOrder;
        co.parameters.spawnid = coordinator.a_u_id;
        co.parameters.factionid = coordinator.a_f_id;
        coordinator.push(co);
    }
    if (ticks == 354)
    {
        isdone = true;

        Unit *u = units[workerChainId];
        if (!u->isIrrigating())
        {
            haspassed = false;
            message = std::string("BuildIrrigationOrder was rejected next to an already-irrigated tile (irrigation chain).");
            return 0;
        }

        haspassed = true;
    }

    return 0;
}
std::string TestCase_038::title()
{
    return std::string("Irrigation placement rules: denied on HILLS/ARCTIC/MOUNTAINS, requires a RIVER/OASIS/LAKE/already-irrigated neighbour.");
}

bool TestCase_038::done()
{
    return isdone;
}
bool TestCase_038::passed()
{
    return haspassed;
}
std::string TestCase_038::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_038();
}
