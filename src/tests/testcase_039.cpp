//  TestCase_039.cpp
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

#include "testcase_039.h"

// @Issue: "Complete the issue for all the improvements" -- task #29's already-irrigated guard
// (mapcell::hasIrrigation(), BuildIrrigationOrder) is now extended to every other improvement
// type: Road, Mine, Railroad, Quarry, Camp, Derrick, Plantation. Each BuildXOrder handler
// (engine.cpp) now rejects (message(), no state change) if the tile already has that SAME
// improvement (mapcell::hasRoad()/hasMine()/hasRailroad()/hasQuarry()/hasCamp()/hasDerrick()/
// hasPlantation() -- hasMine() is new, mapmodel.h, mirroring the others; buildMine() existed
// with no matching getter before this). Deliberately NOT cross-checking between related
// types: a tile with a Road can still receive a Railroad (upgrade path), so hasRoad() is
// only consulted by BuildRoadOrder, not BuildRailroadOrder.
//
// One worker per tile, each tile already carrying that improvement (built directly via the
// mapcell::buildX() setter -- no need to simulate a full multi-turn build here, task #29's
// testcase_038 already covers that mechanism end-to-end for Irrigation) plus whatever
// resource that type needs, so the ONLY thing that can reject the order is the new
// already-built guard, not a missing resource:
//   (1,0) GRASSLAND,              hasRoad()       -> BuildRoadOrder REJECTED
//   (2,0) GRASSLAND,              hasMine()       -> BuildMineOrder REJECTED
//   (3,0) GRASSLAND,              hasRailroad()   -> BuildRailroadOrder REJECTED
//   (4,0) GRASSLAND+MARBLE,       hasQuarry()     -> BuildQuarryOrder REJECTED
//   (5,0) GRASSLAND+DOE,          hasCamp()       -> BuildCampOrder REJECTED
//   (6,0) GRASSLAND+OIL,          hasDerrick()    -> BuildDerrickOrder REJECTED
//   (7,0) GRASSLAND+GRAPES,       hasPlantation() -> BuildPlantationOrder REJECTED
// None of these need to wait for a multi-turn effort cycle (rejection is immediate), so
// unlike testcase_038's full-cycle phase, no fortify()/packUp() turn-isolation is needed here.

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

static int spawnWorker(int lat, int lon)
{
    Worker *w = new Worker();
    w->longitude = lon;
    w->latitude = lat;
    w->id = getNextUnitId();
    w->faction = 0;
    w->availablemoves = w->getUnitMoves();

    units[w->id] = w;
    map.set(lat,lon).setOwnedBy(0);

    return w->id;
}

TestCase_039::TestCase_039()
{

}

TestCase_039::~TestCase_039()
{

}

int TestCase_039::number()
{
    return 39;
}

void TestCase_039::init()
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

    map.set(1,0) = mapcell(LAND);
    map.set(1,0).bioma = GRASSLAND;
    map.set(1,0).buildRoad();

    map.set(2,0) = mapcell(LAND);
    map.set(2,0).bioma = GRASSLAND;
    map.set(2,0).buildMine();

    map.set(3,0) = mapcell(LAND);
    map.set(3,0).bioma = GRASSLAND;
    map.set(3,0).buildRailroad();

    map.set(4,0) = mapcell(LAND);
    map.set(4,0).bioma = GRASSLAND;
    map.set(4,0).resource = MARBLE;
    map.set(4,0).buildQuarry();

    map.set(5,0) = mapcell(LAND);
    map.set(5,0).bioma = GRASSLAND;
    map.set(5,0).resource = DOE;
    map.set(5,0).buildCamp();

    map.set(6,0) = mapcell(LAND);
    map.set(6,0).bioma = GRASSLAND;
    map.set(6,0).resource = OIL;
    map.set(6,0).buildDerrick();

    map.set(7,0) = mapcell(LAND);
    map.set(7,0).bioma = GRASSLAND;
    map.set(7,0).resource = GRAPES;
    map.set(7,0).buildPlantation();

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

    autoEndOfTurn = true;

    workerRoadId       = spawnWorker(1,0);
    workerMineId       = spawnWorker(2,0);
    workerRailroadId   = spawnWorker(3,0);
    workerQuarryId     = spawnWorker(4,0);
    workerCampId       = spawnWorker(5,0);
    workerDerrickId    = spawnWorker(6,0);
    workerPlantationId = spawnWorker(7,0);

    citynames[0] = std::queue<std::string>();
    for(int i=0;i<20;i++)
        citynames[0].push("Kattegate");

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = workerRoadId;
}

int TestCase_039::check(int year)
{
    ticks++;

    if (ticks == 10)
    {
        coordinator.a_u_id = workerRoadId;
        CommandOrder co;
        co.command = Command::BuildRoadOrder;
        coordinator.push(co);
    }
    if (ticks == 13)
    {
        Unit *u = units[workerRoadId];
        if (u->isRoading())
        {
            isdone = true;
            haspassed = false;
            message = std::string("BuildRoadOrder was accepted again on a tile that already has a road.");
            return 0;
        }
    }

    if (ticks == 14)
    {
        coordinator.a_u_id = workerMineId;
        CommandOrder co;
        co.command = Command::BuildMineOrder;
        coordinator.push(co);
    }
    if (ticks == 17)
    {
        Unit *u = units[workerMineId];
        if (u->isMining())
        {
            isdone = true;
            haspassed = false;
            message = std::string("BuildMineOrder was accepted again on a tile that already has a mine.");
            return 0;
        }
    }

    if (ticks == 18)
    {
        coordinator.a_u_id = workerRailroadId;
        CommandOrder co;
        co.command = Command::BuildRailroadOrder;
        coordinator.push(co);
    }
    if (ticks == 21)
    {
        Unit *u = units[workerRailroadId];
        if (u->isRailroading())
        {
            isdone = true;
            haspassed = false;
            message = std::string("BuildRailroadOrder was accepted again on a tile that already has a railroad.");
            return 0;
        }
    }

    if (ticks == 22)
    {
        coordinator.a_u_id = workerQuarryId;
        CommandOrder co;
        co.command = Command::BuildQuarryOrder;
        coordinator.push(co);
    }
    if (ticks == 25)
    {
        Unit *u = units[workerQuarryId];
        if (u->isQuarrying())
        {
            isdone = true;
            haspassed = false;
            message = std::string("BuildQuarryOrder was accepted again on a tile that already has a quarry.");
            return 0;
        }
    }

    if (ticks == 26)
    {
        coordinator.a_u_id = workerCampId;
        CommandOrder co;
        co.command = Command::BuildCampOrder;
        coordinator.push(co);
    }
    if (ticks == 29)
    {
        Unit *u = units[workerCampId];
        if (u->isCamping())
        {
            isdone = true;
            haspassed = false;
            message = std::string("BuildCampOrder was accepted again on a tile that already has a camp.");
            return 0;
        }
    }

    if (ticks == 30)
    {
        coordinator.a_u_id = workerDerrickId;
        CommandOrder co;
        co.command = Command::BuildDerrickOrder;
        coordinator.push(co);
    }
    if (ticks == 33)
    {
        Unit *u = units[workerDerrickId];
        if (u->isDerricking())
        {
            isdone = true;
            haspassed = false;
            message = std::string("BuildDerrickOrder was accepted again on a tile that already has a derrick.");
            return 0;
        }
    }

    if (ticks == 34)
    {
        coordinator.a_u_id = workerPlantationId;
        CommandOrder co;
        co.command = Command::BuildPlantationOrder;
        coordinator.push(co);
    }
    if (ticks == 37)
    {
        isdone = true;

        Unit *u = units[workerPlantationId];
        if (u->isPlanting())
        {
            haspassed = false;
            message = std::string("BuildPlantationOrder was accepted again on a tile that already has a plantation.");
            return 0;
        }

        haspassed = true;
    }

    return 0;
}
std::string TestCase_039::title()
{
    return std::string("Already-built guard extended to every improvement: Road/Mine/Railroad/Quarry/Camp/Derrick/Plantation.");
}

bool TestCase_039::done()
{
    return isdone;
}
bool TestCase_039::passed()
{
    return haspassed;
}
std::string TestCase_039::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_039();
}
