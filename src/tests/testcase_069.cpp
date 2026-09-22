//  TestCase_069.cpp
//  bunmei
//
//  Created by Claude on 21/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <vector>

#include "../map.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../commandorder.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../cityscreenui.h"

#include "testcase_069.h"

// @Task: new Command::DeAssignWorkTileOrder, running City::deAssignWorkingTile() through
// processCommandOrders() like every other city order.
//
// The reason it exists is the point of the test: the AI and (later) a remote player drive the
// game exclusively through the command queue, so any state change a turn can make has to be
// expressible as a command rather than as a direct reach into the City. Shedding a surplus
// working tile was only reachable by calling the method -- endOfYear() does that when a city
// starves and loses population.
//
// Its contract is NOT "stop working tile X" (that is AssignWorkTileOrder, whose toggle
// already deassigns a named tile). It is "give up the one tile this city is no longer
// entitled to": it releases a single worked tile while the city is over its pop+1 allowance,
// and does nothing at all when it is not -- which is what the test pins down, in both
// directions, because a command that silently over-releases would break a city quietly.

extern std::unordered_map<int, std::string> tiles;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Map map;
extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_069::TestCase_069() {}
TestCase_069::~TestCase_069() {}

int TestCase_069::number() { return 69; }

void TestCase_069::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initProductionRates(productionrates);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            mapcell &cell = map.set(lat,lon);
            cell = mapcell(LAND);
            cell.bioma = GRASSLAND;
            cell.setVisible(0);
        }
    assignProductionRates(map);

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255; faction->green = 0; faction->blue = 0;
    faction->autoPlayer = false;
    factions.push_back(faction);

    City *city = new City(&map, 0, getNextCityId(), 0, 0);
    city->setName("Kattegate");
    city->foundedyear = -4000;
    cities[city->id] = city;
    cityid = city->id;

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

    controller.cityid = city->id;
    controller.view = 2;
}

int TestCase_069::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };
    isdone = true;

    City* city = cities[cityid];

    // Pushes the new order for this city, exactly as an AI or a remote player would: a city
    // id and nothing else -- no tile, no active unit.
    auto pushDeAssign = [&]()
    {
        CommandOrder co;
        co.command = Command::DeAssignWorkTileOrder;
        co.parameters.cityid = cityid;
        co.parameters.factionid = city->faction;
        coordinator.push(co);
    };

    // ---- 1) a city inside its allowance loses nothing ------------------------------------
    // reSetCities()/the constructor leave the city working its entitled tiles, so the order
    // must be a no-op here. This is the half that catches an over-eager implementation.
    {
        city->pop = 4;
        const int before = city->numberOfWorkingTiles();
        if (before > city->pop + 1)
        { fail("Setup: the city already works more tiles than its population allows."); return 0; }

        pushDeAssign();
        processCommandOrders();

        if (city->numberOfWorkingTiles() != before)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"A city within its allowance went from %d worked tiles to %d -- the order "
                                     "must release nothing while pop+1 is not exceeded.",
                     before, city->numberOfWorkingTiles());
            fail(buf); return 0;
        }
    }

    // ---- 2) a city over its allowance sheds exactly one tile ------------------------------
    // Assign a spread of tiles, then drop the population so the city is over-committed --
    // the shape endOfYear() produces when a city starves.
    {
        const int ring[][2] = { {1,1}, {-1,-1}, {1,-1}, {-1,1}, {2,2}, {-2,-2} };
        for (const auto& t : ring)
            if (!city->workingOn(t[0], t[1]))
                city->assignWorkingTile(coordinate(t[0], t[1]));

        const int worked = city->numberOfWorkingTiles();
        if (worked < 4)
        { fail("Setup: could not get the city working enough tiles to be over its allowance."); return 0; }

        city->pop = 1;                        // allowance is now pop+1 = 2, well below `worked`

        pushDeAssign();
        processCommandOrders();

        const int afterOne = city->numberOfWorkingTiles();
        if (afterOne != worked - 1)
        {
            char buf[220];
            snprintf(buf,sizeof(buf),"One DeAssignWorkTileOrder took %d worked tiles to %d; it must release "
                                     "exactly one (from %d).", worked, afterOne, worked);
            fail(buf); return 0;
        }

        // Issued repeatedly it keeps shedding, one per order, and then STOPS on its own once
        // the city is back inside its allowance -- it must not strip the city bare.
        for (int i=0;i<12;i++)
        {
            pushDeAssign();
            processCommandOrders();
        }

        const int settled = city->numberOfWorkingTiles();
        if (settled > city->pop + 1)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"After repeated orders the city still works %d tiles, more than its pop+1 = %d.",
                     settled, city->pop + 1);
            fail(buf); return 0;
        }
        if (settled == 0)
        { fail("Repeated orders stripped the city of every worked tile, including its centre."); return 0; }
    }

    // ---- 3) the order is routed by CITY ID, not by the active unit ------------------------
    // Every city order has to run before processCommandOrders()'s active-unit guard, or it
    // would be dropped for a faction whose cursor is on no unit -- which is exactly the state
    // an AI or a remote player issues city orders from. coordinator.a_u_id is CONTROLLING_NONE
    // for this whole test, so everything above already ran through that path; this checks the
    // other half: an id that names no city is ignored rather than crashing or hitting cities
    // at random.
    {
        const int before = city->numberOfWorkingTiles();

        CommandOrder co;
        co.command = Command::DeAssignWorkTileOrder;
        co.parameters.cityid = 99999;         // no such city
        co.parameters.factionid = 0;
        coordinator.push(co);
        processCommandOrders();

        if (city->numberOfWorkingTiles() != before)
        { fail("An order naming a city that does not exist affected a different city."); return 0; }
        if (cities.find(99999) != cities.end())
        { fail("Looking up a missing city id inserted it -- use find(), never cities[id]."); return 0; }
    }

    // ---- 4) a surplus that sits entirely on the centre row/column ------------------------
    // The guard in deAssignWorkingTile() used to be `lat!=0 && lon!=0`, which skips every
    // tile on the centre ROW OR COLUMN, not just the centre -- so a city whose surplus
    // happened to lie on an axis could never shed anything and stayed over its allowance
    // forever, with the order looking like a silent no-op. Worked tiles here are ONLY axis
    // ones, which is the case the old guard could not touch.
    {
        // Start clean: give up everything the earlier sections left assigned.
        for (int lat=-3;lat<=3;lat++)
            for (int lon=-3;lon<=3;lon++)
                if (!(lat==0 && lon==0) && city->workingOn(lat,lon))
                    city->deAssignTile(coordinate(lat,lon));

        city->pop = 6;                        // room to assign the ring below
        const int axis[][2] = { {1,0}, {0,1}, {2,0}, {0,-1}, {-1,0} };
        for (const auto& t : axis)
            city->assignTile(coordinate(t[0], t[1]));

        const int worked = city->numberOfWorkingTiles();
        if (worked < 4)
        { fail("Setup: could not get the city working a set of axis tiles."); return 0; }
        for (int lat=-3;lat<=3;lat++)
            for (int lon=-3;lon<=3;lon++)
                if (city->workingOn(lat,lon) && lat != 0 && lon != 0)
                { fail("Setup: an off-axis tile is worked, which the old guard COULD release -- the case would not be proven."); return 0; }

        city->pop = 1;                        // allowance 2, well under `worked`

        pushDeAssign();
        processCommandOrders();

        if (city->numberOfWorkingTiles() != worked - 1)
        {
            char buf[240];
            snprintf(buf,sizeof(buf),"With every surplus tile on the centre row/column, the order took %d worked "
                                     "tiles to %d -- it must still release one (the centre alone is exempt).",
                     worked, city->numberOfWorkingTiles());
            fail(buf); return 0;
        }

        // ...and it keeps going until the city is back inside its allowance, without ever
        // giving up the centre.
        for (int i=0;i<12;i++)
        {
            pushDeAssign();
            processCommandOrders();
        }
        if (city->numberOfWorkingTiles() > city->workingTileAllowance())
        { fail("An axis-only surplus never settled back inside the allowance."); return 0; }
        if (!city->workingOn(0,0))
        { fail("The city gave up its own centre tile."); return 0; }
    }

    haspassed = true;
    return 0;
}

std::string TestCase_069::title()
{
    return std::string("New Command::DeAssignWorkTileOrder runs City::deAssignWorkingTile() through processCommandOrders(), addressed by city id with no active unit -- so the AI and a remote player can shed a city's surplus working tile through the command queue instead of reaching into the City. Releases exactly one tile per order while the city is over pop+1, nothing once it is not, and (guard fix) works when the whole surplus sits on the centre row or column.");
}

bool TestCase_069::done()   { return isdone; }
bool TestCase_069::passed() { return haspassed; }
std::string TestCase_069::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_069();
}
