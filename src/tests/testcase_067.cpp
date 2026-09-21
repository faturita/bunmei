//  TestCase_067.cpp
//  bunmei
//
//  Created by Claude on 21/09/2026
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <vector>

#include "../map.h"
#include "../units/Unit.h"
#include "../units/Warrior.h"
#include "../units/Worker.h"
#include "../units/Settler.h"
#include "../units/Trireme.h"
#include "../units/Galleon.h"
#include "../City.h"
#include "../Faction.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../improvements.h"
#include "../usercontrols.h"
#include "../mapio.h"
#include "../savegame.h"

#include "testcase_067.h"

// @Task: persist each unit's STATUS -- fortified, sentried, automated (with where it was
// sent), and which improvement it is building -- and everything aboard a ship.
//
// Two things were simply absent from the savegame before this: unit status entirely, and
// cargo. Cargo split into two different failures, which is why both are checked here:
//   * a boarded UNIT was saved (it stays in the global units map while aboard) but the fact
//     that it was ABOARD was not, so it came back as a land unit standing on open water;
//   * a boarded RESOURCE stack exists nowhere except in the ship's slot, so it was lost
//     outright.
//
// Work COUNTERS are deliberately not saved (faturita's call): a unit comes back still doing
// the same job, but from the full effort its tile requires. So the test spends part of the
// effort before saving and then measures how many turns the reloaded unit needs -- the whole
// requirement, not the remainder.
//
// Driven through the real format and the real load order (savegame(), then loadMap, year,
// loadCities, loadUnits, loadDependencies, loadTechnologies, loadUnitStatus) so the block
// ordering is exercised too, not just the field encoding.

extern Map map;
extern std::unordered_map<int, std::string> tiles;
extern std::unordered_map<int, Improvement*> improvements;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern ImprovementEffort improvementeffort;
extern float mapzoom;

extern Coordinator coordinator;
extern int year;

#define TEST_MAPSIZE 1

// The worker irrigates here; GRASSLAND, so the effort is the table's flat 9.
#define WORKER_LAT   2
#define WORKER_LON   0
#define SHIP_LAT     6      // ocean, so a passenger left unboarded would be standing on water
#define SHIP_LON     0
#define EFFORT_SPENT 4      // turns of irrigation done BEFORE saving, to be discarded

// Cargo aboard the Trireme: one passenger unit and one resource stack.
#define CARGO_RESOURCE  iron
#define CARGO_AMOUNT    70

TestCase_067::TestCase_067() {}
TestCase_067::~TestCase_067() {}

int TestCase_067::number() { return 67; }

void TestCase_067::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initProductionRates(productionrates);
    initImprovements(improvements);
    initImprovementEffort(improvementeffort);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            mapcell &cell = map.set(lat,lon);
            cell = mapcell(lat >= 5 ? OCEAN : LAND);
            cell.bioma = (lat >= 5) ? OCEANBIOMA : GRASSLAND;
            cell.setVisible(0);
        }
    assignProductionRates(map);

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255; faction->green = 0; faction->blue = 0;
    faction->autoPlayer = false;
    factions.push_back(faction);

    citynames[0] = std::queue<std::string>();

    // A city, because savegame() writes the city block before the units and the loader has
    // to walk through it to reach them.
    City *city = new City(&map, 0, getNextCityId(), 0, 0);
    city->setName("Kattegate");
    city->setCapitalCity();
    cities[city->id] = city;
    citynames[0].push("Kattegate");

    auto place = [&](Unit* u, int lat, int lon)
    {
        u->id        = getNextUnitId();
        u->faction   = 0;
        u->latitude  = lat;
        u->longitude = lon;
        u->availablemoves = u->getUnitMoves();
        units[u->id] = u;
        return u;
    };

    // 1. A worker part-way through irrigating.
    Worker* worker = (Worker*)place(new Worker(), WORKER_LAT, WORKER_LON);
    workerId = worker->id;
    worker->irrigating(getImprovementEffort(improvementeffort, IRRIGATION, GRASSLAND));
    worker->availablemoves = 1;
    for (int i=0;i<EFFORT_SPENT;i++)
        worker->work();                       // burn some effort, to be thrown away on load

    // 2. Fortified. 3. Sentried. 4. Automated, heading somewhere specific.
    fortifiedId = place(new Warrior(), 1, 1)->id;
    units[fortifiedId]->fortify();

    sentriedId = place(new Warrior(), 1, 2)->id;
    units[sentriedId]->sentry();

    autoId = place(new Settler(), 1, 3)->id;
    units[autoId]->goTo(-3, -4);

    // 5. A ship carrying a unit AND a resource stack.
    Trireme* trireme = (Trireme*)place(new Trireme(), SHIP_LAT, SHIP_LON);
    shipId = trireme->id;

    Warrior* passenger = (Warrior*)place(new Warrior(), SHIP_LAT, SHIP_LON);
    passengerId = passenger->id;
    trireme->board(dynamic_cast<Shippable*>(passenger));
    passenger->sentry();                      // what moveOntoNavalUnit() does to a passenger

    Commodity* cargo = new Commodity(CARGO_RESOURCE, tiles[CARGO_RESOURCE].c_str(), "Commodity");
    cargo->amount = CARGO_AMOUNT;
    trireme->board(dynamic_cast<Shippable*>(cargo));

    char namebuf[64];
    snprintf(namebuf, sizeof(namebuf), "testcase067_%d", (int)((time(nullptr) ^ getpid()) & 0xffffff));
    savename = std::string("saves/") + namebuf;
    savegame(savename.c_str());

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
}

int TestCase_067::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 5) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };
    isdone = true;

    const int fullEffort = getImprovementEffort(improvementeffort, IRRIGATION, GRASSLAND);

    // Setup sanity: the worker really did spend effort, so "restarted" is distinguishable
    // from "resumed".
    if (!units[workerId]->isIrrigating())
    { fail("Setup: the worker should be irrigating before the save."); return 0; }

    // ---- wipe: only the file may restore any of this --------------------------------------
    for (auto& [k, u] : units) delete u;
    units.clear();
    for (auto& [k, c] : cities) delete c;
    cities.clear();
    citynames[0] = std::queue<std::string>();
    citynames[0].push("Kattegate");

    // ---- read back, in the order gamekernel.cpp:loadWorldModelling() uses -----------------
    loadMap(savename + ".map");

    // Through readSaveGame(), like the game does (general header + verified payload).
    std::string savedata;
    SaveGameInfo saveinfo;
    if (!readSaveGame(savename.c_str(), savedata, saveinfo))
    { fail("readSaveGame() rejected the file this test just wrote."); return 0; }
    std::istringstream in(savedata, std::ios::binary);

    int loadedYear = 0;
    in.read(reinterpret_cast<char*>(&loadedYear), sizeof(loadedYear));
    loadCities(in);
    loadUnits(in);
    loadDependencies(in);
    loadTechnologies(in);
    loadUnitStatus(in);

    auto unit = [&](int id)->Unit*
    {
        auto it = units.find(id);
        return it == units.end() ? nullptr : it->second;
    };

    for (int id : { workerId, fortifiedId, sentriedId, autoId, shipId, passengerId })
        if (unit(id) == nullptr)
        {
            char buf[160];
            snprintf(buf,sizeof(buf),"Unit %d did not come back from the savegame at all.", id);
            fail(buf); return 0;
        }

    // ---- 1) the passive states ------------------------------------------------------------
    if (!unit(fortifiedId)->isFortified())
    { fail("A fortified unit came back un-fortified."); return 0; }
    if (!unit(sentriedId)->isSentry())
    { fail("A sentried unit came back off sentry."); return 0; }
    // ...and they must not bleed into each other.
    if (unit(fortifiedId)->isSentry() || unit(sentriedId)->isFortified())
    { fail("The fortified and sentried flags were crossed on load."); return 0; }
    if (unit(workerId)->isFortified() || unit(workerId)->isSentry())
    { fail("The worker was neither fortified nor sentried -- it must not come back either."); return 0; }

    // ---- 2) automated, and still heading for the same tile --------------------------------
    if (!unit(autoId)->isAuto())
    { fail("An automated unit came back off automatic."); return 0; }
    if (unit(autoId)->target.lat != -3 || unit(autoId)->target.lon != -4)
    {
        char buf[200];
        snprintf(buf,sizeof(buf),"The automated unit came back heading for (%d,%d), it was sent to (-3,-4).",
                 unit(autoId)->target.lat, unit(autoId)->target.lon);
        fail(buf); return 0;
    }

    // ---- 3) still irrigating, but from scratch --------------------------------------------
    Unit* worker = unit(workerId);
    if (!worker->isIrrigating())
    { fail("The worker came back idle -- the improvement it was building must survive."); return 0; }
    if (worker->isRoading() || worker->isMining() || worker->isRailroading() ||
        worker->isQuarrying() || worker->isCamping() || worker->isDerricking() || worker->isPlanting())
    { fail("The worker came back building more than one improvement -- the work kind was decoded wrong."); return 0; }

    // Measured, not assumed: one effort per turn, counted until the job finishes. It must be
    // the FULL requirement, not the requirement minus what was spent before saving.
    worker->availablemoves = 1;
    int turns = 0;
    while (turns < 1000 && worker->isWorking())
    {
        worker->work();
        turns++;
    }
    if (turns != fullEffort)
    {
        char buf[240];
        snprintf(buf,sizeof(buf),"The reloaded worker finished irrigating in %d turns; the tile takes %d from "
                                 "scratch (it had already spent %d before the save, which must be discarded).",
                 turns, fullEffort, EFFORT_SPENT);
        fail(buf); return 0;
    }

    // ---- 4) the ship's cargo: a passenger AND a resource stack ----------------------------
    Transport* ship = dynamic_cast<Transport*>(unit(shipId));
    if (ship == nullptr)
    { fail("The Trireme came back as something that cannot carry cargo."); return 0; }

    std::vector<Shippable*> cargo = ship->getCargo();
    if ((int)cargo.size() != 2)
    {
        char buf[200];
        snprintf(buf,sizeof(buf),"The Trireme came back with %d cargo item(s); it was saved carrying 2 "
                                 "(a passenger and a resource stack).", (int)cargo.size());
        fail(buf); return 0;
    }

    Unit*     foundPassenger = nullptr;
    Resource* foundResource  = nullptr;
    for (Shippable* s : cargo)
    {
        Unit* asUnit = dynamic_cast<Unit*>(s);
        if (asUnit != nullptr) foundPassenger = asUnit;
        else                   foundResource  = dynamic_cast<Resource*>(s);
    }

    if (foundPassenger == nullptr)
    { fail("The boarded unit is not aboard after the load -- it would be standing on open water."); return 0; }
    if (foundPassenger->id != passengerId)
    { fail("The wrong unit was re-boarded onto the Trireme."); return 0; }
    // The SAME object the units map holds, not a copy: a duplicate would move independently.
    if (foundPassenger != unit(passengerId))
    { fail("The re-boarded passenger is a different object from the one in the units map."); return 0; }

    if (foundResource == nullptr)
    { fail("The resource cargo is gone -- it exists nowhere but aboard, so nothing else could restore it."); return 0; }
    if (foundResource->id != CARGO_RESOURCE || foundResource->amount != CARGO_AMOUNT)
    {
        char buf[200];
        snprintf(buf,sizeof(buf),"The resource cargo came back as 0x%x x%d, saved as 0x%x x%d.",
                 foundResource->id, foundResource->amount, CARGO_RESOURCE, CARGO_AMOUNT);
        fail(buf); return 0;
    }

    // A unit that carries nothing must not acquire cargo from someone else's record.
    Transport* notAShip = dynamic_cast<Transport*>(unit(fortifiedId));
    if (notAShip != nullptr && notAShip->manifest() != 0)
    { fail("A unit that was carrying nothing came back with cargo."); return 0; }

    haspassed = true;
    return 0;
}

std::string TestCase_067::title()
{
    return std::string("Savegame now carries unit STATUS and ship CARGO: fortified/sentried/automated (with its destination) and which improvement a unit is building -- restarted from full effort, since the work counters are deliberately not saved -- plus every boarded unit (re-boarded by id) and resource stack (rebuilt) aboard a Transport.");
}

bool TestCase_067::done()   { return isdone; }
bool TestCase_067::passed() { return haspassed; }
std::string TestCase_067::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_067();
}
