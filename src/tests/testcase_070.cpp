//  TestCase_070.cpp
//  bunmei
//
//  Created by Claude on 22/09/2026
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <vector>

#include "../map.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../coordinator.h"
#include "../commandorder.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"

#include "testcase_070.h"

// @Task: Command::AssignTileOrder / Command::DeAssignTileOrder -- work, or stop working, ONE
// NAMED tile of a city.
//
// These are the explicit counterparts to AssignWorkTileOrder, which is a TOGGLE. The toggle
// requires the sender to already know the tile's current state: fine for a UI click, wrong
// for the AI and for a remote player whose view of the city can be a turn stale. Two toggles
// racing on one tile cancel out; a stale view flips the wrong way.
//
// So each of these states an intended OUTCOME and does nothing when it already holds, which
// makes them idempotent -- sending one twice is the same as sending it once. The four no-op
// rules, all checked below:
//   * deassign a tile that is not assigned  -> nothing
//   * assign a tile that is already assigned -> nothing
//   * assign with no assignment left (numberOfWorkingTiles() == pop+1) -> nothing
//   * either one on the centre, out of range, or a tile another city/faction holds -> nothing
//
// Everything goes through coordinator.push() + processCommandOrders() with no active unit
// (a_u_id == CONTROLLING_NONE), because that is the state an AI or a remote player issues
// city orders from.

extern std::unordered_map<int, std::string> tiles;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Map map;
extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_070::TestCase_070() {}
TestCase_070::~TestCase_070() {}

int TestCase_070::number() { return 70; }

void TestCase_070::init()
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

    for (int f=0;f<2;f++)
    {
        Faction *faction = new Faction();
        faction->id = f;
        strcpy(faction->name, f == 0 ? "Vikings" : "Romans");
        faction->red = 255; faction->green = 0; faction->blue = 0;
        faction->autoPlayer = false;
        factions.push_back(faction);
        citynames[f] = std::queue<std::string>();
    }

    City *city = new City(&map, 0, getNextCityId(), 0, 0);
    city->setName("Kattegate");
    city->foundedyear = -4000;
    cities[city->id] = city;
    cityid = city->id;

    // A SECOND city, of another faction, INSIDE this city's 7x7 range (offset (0,3), so
    // Kattegate's relative (0,3) is Roma's own centre): its tiles are the "someone else
    // holds it" case. It has to be within -3..3 or the range check would refuse first and
    // the occupancy rule would never be reached.
    City *rival = new City(&map, 1, getNextCityId(), 0, 3);
    rival->setName("Roma");
    rival->foundedyear = -4000;
    cities[rival->id] = rival;
    rivalid = rival->id;

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

    controller.cityid = city->id;
    controller.view = 2;
}

int TestCase_070::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };
    isdone = true;

    City* city  = cities[cityid];
    City* rival = cities[rivalid];

    auto order = [&](Command cmd, int lat, int lon)
    {
        CommandOrder co;
        co.command = cmd;
        co.parameters.cityid = cityid;
        co.parameters.factionid = city->faction;
        co.parameters.latitude  = lat;      // RELATIVE to the city, -3..3
        co.parameters.longitude = lon;
        coordinator.push(co);
        processCommandOrders();
    };

    // Clean slate: only the centre worked, and plenty of allowance.
    for (int lat=-3;lat<=3;lat++)
        for (int lon=-3;lon<=3;lon++)
            if (!(lat==0 && lon==0) && city->workingOn(lat,lon))
                city->deAssignTile(coordinate(lat,lon));
    city->pop = 4;                          // allowance 5, centre already takes 1

    if (city->numberOfWorkingTiles() != 1 || !city->workingOn(0,0))
    { fail("Setup: the city should start working only its centre tile."); return 0; }

    // ---- 1) assigning a free tile works, and is idempotent -------------------------------
    order(Command::AssignTileOrder, 1, 1);
    if (!city->workingOn(1,1))
    { fail("AssignTileOrder did not put the city to work on a free tile in range."); return 0; }
    if (city->numberOfWorkingTiles() != 2)
    { fail("AssignTileOrder changed more than the one tile it named."); return 0; }

    // Re-assigning has to be a true no-op, not a harmless-looking repeat: setCityOwnership()
    // on a tile this faction already owns INCREMENTS mapcell::owners (the stacking counter),
    // which workingOn() cannot see. So watch the counter, not just the flag.
    const int ownersBefore = map.peek(city->latitude+1, city->longitude+1).owners;
    order(Command::AssignTileOrder, 1, 1);      // again
    if (!city->workingOn(1,1) || city->numberOfWorkingTiles() != 2)
    { fail("Assigning an ALREADY assigned tile must do nothing -- it is not a toggle."); return 0; }
    if (map.peek(city->latitude+1, city->longitude+1).owners != ownersBefore)
    {
        char buf[200];
        snprintf(buf,sizeof(buf),"Re-assigning an already worked tile moved its owners counter %d -> %d; "
                                 "the order must not touch the tile at all.",
                 ownersBefore, map.peek(city->latitude+1, city->longitude+1).owners);
        fail(buf); return 0;
    }

    // ---- 2) deassigning that tile works, and is idempotent -------------------------------
    order(Command::DeAssignTileOrder, 1, 1);
    if (city->workingOn(1,1))
    { fail("DeAssignTileOrder did not take the city off the tile it named."); return 0; }
    if (city->numberOfWorkingTiles() != 1)
    { fail("DeAssignTileOrder released more than the one tile it named."); return 0; }

    order(Command::DeAssignTileOrder, 1, 1);    // again
    if (city->workingOn(1,1) || city->numberOfWorkingTiles() != 1)
    { fail("Deassigning a tile that is NOT assigned must do nothing -- it is not a toggle."); return 0; }

    // ---- 3) assigning with no assignment left does nothing --------------------------------
    // Fill the city up to its allowance, then ask for one more.
    {
        const int spare[][2] = { {1,1}, {2,2}, {-1,-1}, {-2,-2}, {1,-1}, {-1,1}, {2,-2} };
        for (const auto& t : spare)
            order(Command::AssignTileOrder, t[0], t[1]);

        const int worked = city->numberOfWorkingTiles();
        if (worked != city->workingTileAllowance())
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"Filling up gave %d worked tiles, the allowance is %d -- assignment must "
                                     "stop exactly at pop+1.", worked, city->workingTileAllowance());
            fail(buf); return 0;
        }
        if (city->workingOn(2,-2))
        { fail("The order that would have exceeded the allowance was applied anyway."); return 0; }

        // One more, on a tile that is free and in range: refused purely for lack of allowance.
        order(Command::AssignTileOrder, 3, 3);
        if (city->workingOn(3,3))
        { fail("AssignTileOrder assigned a tile with no assignment left."); return 0; }
        if (city->numberOfWorkingTiles() != worked)
        { fail("A refused assignment still changed the number of worked tiles."); return 0; }

        // Freeing one makes room for exactly one more -- the allowance is a live check, not
        // a one-off at setup.
        order(Command::DeAssignTileOrder, 1, 1);
        order(Command::AssignTileOrder, 3, 3);
        if (!city->workingOn(3,3))
        { fail("After freeing a tile, the next assignment should fit."); return 0; }
        if (city->numberOfWorkingTiles() != worked)
        { fail("Freeing one and assigning one should leave the count unchanged."); return 0; }
    }

    // ---- 4) the centre, out of range, and another city's land -----------------------------
    {
        const int before = city->numberOfWorkingTiles();

        order(Command::DeAssignTileOrder, 0, 0);
        if (!city->workingOn(0,0))
        { fail("The city gave up its own centre tile -- (0,0) is never deassignable."); return 0; }

        order(Command::AssignTileOrder, 0, 0);
        if (city->numberOfWorkingTiles() != before)
        { fail("Assigning the centre tile changed the worked-tile count -- it is already worked, always."); return 0; }

        // Out of the 7x7 range entirely.
        order(Command::AssignTileOrder, 4, 0);
        order(Command::AssignTileOrder, 0, -7);
        order(Command::DeAssignTileOrder, 4, 0);
        if (city->numberOfWorkingTiles() != before)
        { fail("An out-of-range tile offset must be ignored, not wrapped or clamped onto a real tile."); return 0; }

        // A tile the RIVAL city already works. Roma sits at (0,3), so Roma's own centre is
        // this city's relative (0,3) -- in range for Kattegate, and held by Roma.
        if (!rival->workingOn(0,0))
        { fail("Setup: the rival city should be working its own centre tile."); return 0; }
        if (map.peek(city->latitude+0, city->longitude+3).f_id_owner != rival->faction)
        { fail("Setup: the rival's centre is not where this test thinks it is."); return 0; }

        city->pop = 8;                      // make sure allowance is NOT what refuses it

        order(Command::AssignTileOrder, 0, 3);
        if (city->workingOn(0,3))
        { fail("AssignTileOrder took a tile that another faction's city already holds."); return 0; }
        if (!rival->workingOn(0,0) || map.peek(city->latitude+0, city->longitude+3).f_id_owner != rival->faction)
        { fail("The refused assignment still overwrote the rival city's ownership of that tile."); return 0; }

        // And deassigning a tile this city does not work must not strip it from whoever does.
        order(Command::DeAssignTileOrder, 0, 3);
        if (!rival->workingOn(0,0) || map.peek(city->latitude+0, city->longitude+3).f_id_owner != rival->faction)
        { fail("Deassigning a tile this city does not work released it from the city that DOES."); return 0; }
    }

    // ---- 5) an order naming no city is ignored -------------------------------------------
    {
        const int before = city->numberOfWorkingTiles();

        CommandOrder co;
        co.command = Command::AssignTileOrder;
        co.parameters.cityid = 99999;
        co.parameters.factionid = 0;
        co.parameters.latitude = 2; co.parameters.longitude = 0;
        coordinator.push(co);
        processCommandOrders();

        if (city->numberOfWorkingTiles() != before)
        { fail("An order naming a city that does not exist affected a different city."); return 0; }
        if (cities.find(99999) != cities.end())
        { fail("Looking up a missing city id inserted it -- use find(), never cities[id]."); return 0; }
    }

    haspassed = true;
    return 0;
}

std::string TestCase_070::title()
{
    return std::string("New Command::AssignTileOrder / DeAssignTileOrder work or stop working ONE NAMED city tile, explicitly rather than by toggling: each does nothing when its outcome already holds (already assigned / not assigned / no assignment left at pop+1 / the centre / out of range / another city's land), so both are idempotent and safe for an AI or a remote player with a stale view.");
}

bool TestCase_070::done()   { return isdone; }
bool TestCase_070::passed() { return haspassed; }
std::string TestCase_070::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_070();
}
