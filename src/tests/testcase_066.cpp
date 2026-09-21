//  TestCase_066.cpp
//  bunmei
//
//  Created by Claude on 21/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <vector>

#include "../map.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../improvements.h"
#include "../usercontrols.h"
#include "../mapio.h"

#include "testcase_066.h"

// @Issue: saveMap() stored each tile's production rates -- and stored the wrong number.
// cell.getResourceProductionRate(i) has the IMPROVEMENT factor already applied, while
// loadMap() read the value straight back as the tile's BASE, with the improvements bitmap
// restored alongside it, so the bonus applied a second time on read and compounded on the
// next save (an irrigated RIVER tile: FOOD 2 -> 4 -> 8 -> 16). saves/game.map really did
// hold its one irrigated tile, (-35,44), at FOOD 4 instead of 2. The live game escaped it
// only because initMap() happened to re-assign every rate as its last statement, after the
// load -- nothing about the design guaranteed that, and no other loader did it.
//
// Fix, and the point of this test: tiles KEEP their own rates -- that is deliberate, a single
// tile can be given a yield nothing else on the map has -- but the rates are never saved.
// assignProductionRates() (engine.cpp, one shared copy now) fills them from the
// productionrates tables, and loadMap() calls it itself, so a generated world and a loaded
// one end up in the same state by the same code and the file only carries what a tile IS.
//
// Round-tripped TWICE, because once is not enough to catch compounding: the first cycle is
// where the old code doubled, the second is where it doubled again.

extern Map map;
extern std::unordered_map<int, std::string> tiles;
extern std::unordered_map<int, Improvement*> improvements;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::vector<Faction*> factions;
extern float mapzoom;

extern Coordinator coordinator;

#define TEST_MAPSIZE 1

// The tiles under test, and what the tables say each must yield -- forever, however many
// times the map is written and read back.
//   plain RIVER            : base FOOD 2
//   RIVER + irrigation     : FOOD 2 * 2.0 = 4        <- the tile that used to compound
//   GRASSLAND + GEMS       : CULTURE 2 (a resource override, no improvement)
//   GRASSLAND + GEMS + road: CULTURE 2 * 2.0 = 4     <- resource AND improvement together
#define RIVER_LAT   1
#define RIVER_LON   1
#define IRRIG_LAT   2
#define IRRIG_LON   1
#define GEMS_LAT    3
#define GEMS_LON    1
#define GEMSROAD_LAT 4
#define GEMSROAD_LON 1

#define EXPECT_RIVER_FOOD     2
#define EXPECT_IRRIG_FOOD     4
#define EXPECT_GEMS_CULTURE   2
#define EXPECT_GEMSROAD_CULTURE 4

TestCase_066::TestCase_066() {}
TestCase_066::~TestCase_066() {}

int TestCase_066::number() { return 66; }

void TestCase_066::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initProductionRates(productionrates);
    initImprovements(improvements);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            mapcell &cell = map.set(lat,lon);
            cell = mapcell(LAND);
            cell.bioma = GRASSLAND;
            cell.setVisible(0);
        }

    map.set(RIVER_LAT,RIVER_LON).bioma = RIVER;

    mapcell &irrigated = map.set(IRRIG_LAT,IRRIG_LON);
    irrigated.bioma        = RIVER;
    irrigated.improvements = IRRIGATION;

    map.set(GEMS_LAT,GEMS_LON).resource = GEMS;

    mapcell &gemsroad = map.set(GEMSROAD_LAT,GEMSROAD_LON);
    gemsroad.resource     = GEMS;
    gemsroad.improvements = ROAD;

    // Terrain and resources are in place; now every tile gets its own rates from the tables.
    assignProductionRates(map);

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255; faction->green = 0; faction->blue = 0;
    faction->autoPlayer = false;
    factions.push_back(faction);

    citynames[0] = std::queue<std::string>();

    char namebuf[64];
    snprintf(namebuf, sizeof(namebuf), "saves/testcase066_%d.map", (int)((time(nullptr) ^ getpid()) & 0xffffff));
    savename = std::string(namebuf);

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
}

int TestCase_066::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };
    isdone = true;

    // Every yield the round trip has to preserve, checked in one place so the same four
    // numbers are demanded before saving and after every load.
    auto verify = [&](const char* when)->bool
    {
        struct Case { int lat; int lon; int resource; int expected; const char* what; };
        const Case cases[] = {
            { RIVER_LAT,    RIVER_LON,    FOOD,    EXPECT_RIVER_FOOD,       "plain RIVER FOOD"            },
            { IRRIG_LAT,    IRRIG_LON,    FOOD,    EXPECT_IRRIG_FOOD,       "irrigated RIVER FOOD"        },
            { GEMS_LAT,     GEMS_LON,     CULTURE, EXPECT_GEMS_CULTURE,     "GEMS CULTURE"                },
            { GEMSROAD_LAT, GEMSROAD_LON, CULTURE, EXPECT_GEMSROAD_CULTURE, "GEMS+road CULTURE"           }
        };

        for (const Case& c : cases)
        {
            int got = map.peek(c.lat,c.lon).getResourceProductionRate(c.resource);
            if (got != c.expected)
            {
                char buf[240];
                snprintf(buf,sizeof(buf),"%s: %s is %d, expected %d. A yield must be derived from the "
                                         "tile, never carried in the file and re-adjusted.",
                         when, c.what, got, c.expected);
                fail(buf);
                return false;
            }
        }
        return true;
    };

    // ---- 0) the tables produce the right numbers to begin with ---------------------------
    if (!verify("before saving")) return 0;

    // ---- 1) first save/load cycle ---------------------------------------------------------
    // This is where the old code doubled: it wrote 4 for the irrigated tile (2 * irrigation)
    // and read that 4 back as the base, which then read out as 8.
    saveMap(savename);
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon) = mapcell(OCEAN);          // wipe, so only the file can restore it

    loadMap(savename);
    if (!verify("after one save/load")) return 0;

    // The three fields the yield is derived FROM are what actually has to survive.
    if (map.peek(IRRIG_LAT,IRRIG_LON).bioma != RIVER ||
        (map.peek(IRRIG_LAT,IRRIG_LON).improvements & IRRIGATION) == 0)
    { fail("The irrigated tile's bioma and improvements must survive the round trip -- they are the source of its yield."); return 0; }
    if (map.peek(GEMS_LAT,GEMS_LON).resource != GEMS)
    { fail("The gem tile's special resource must survive the round trip."); return 0; }

    // ---- 2) second cycle: the one that proves it does not COMPOUND -----------------------
    saveMap(savename);
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon) = mapcell(OCEAN);

    loadMap(savename);
    if (!verify("after two save/load cycles")) return 0;

    // ---- 3) and the file no longer carries the derived rates at all ----------------------
    // mapio.cpp still writes the block's COUNT, always 0, because the map format has no
    // version header and a pre-change file has to keep loading (its stale rates are read and
    // discarded). What must not happen is a rate being written back out.
    {
        std::ifstream in(savename, std::ios::binary);
        if (!in) { fail("Could not reopen the saved map to inspect its format."); return 0; }

        int lat=0, lon=0, code=0, bioma=0, resource=0;
        size_t sz = 1;
        in.read(reinterpret_cast<char*>(&lat), sizeof(lat));
        in.read(reinterpret_cast<char*>(&lon), sizeof(lon));
        in.read(reinterpret_cast<char*>(&code), sizeof(code));
        in.read(reinterpret_cast<char*>(&bioma), sizeof(bioma));
        in.read(reinterpret_cast<char*>(&resource), sizeof(resource));
        in.read(reinterpret_cast<char*>(&sz), sizeof(sz));
        in.close();

        if (sz != 0)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"The saved map still stores %d production rate(s) per tile; "
                                     "a derived value must not be persisted.", (int)sz);
            fail(buf); return 0;
        }
    }

    // ---- 4) tiles still have their OWN rates, and one can be singled out -----------------
    // The rates being table-FILLED must not be confused with their being table-BOUND: the
    // vector is per-tile so that a scenario, an event or a test can give one tile a yield
    // nothing else on the map has. That flexibility is the reason the vector exists at all.
    {
        mapcell &special = map.set(RIVER_LAT,RIVER_LON);
        special.setResourceProductionRate(FOOD, 9);

        if (special.getResourceProductionRate(FOOD) != 9)
        { fail("A per-tile rate must be settable on its own -- one tile, one yield, no bioma anywhere else touched."); return 0; }
        if (map.peek(IRRIG_LAT,IRRIG_LON).getResourceProductionRate(FOOD) != EXPECT_IRRIG_FOOD)
        { fail("Changing one tile's rate must not disturb any other tile."); return 0; }

        // Improvements still multiply a hand-set base, same as a table-set one.
        special.improvements |= IRRIGATION;
        if (special.getResourceProductionRate(FOOD) != 18)
        { fail("Irrigation should double a hand-set base too (9 -> 18): the factor is applied live, not baked in."); return 0; }
        special.improvements &= ~IRRIGATION;

        // It is RUNTIME state, though: nothing in the file explains a yield of 9, so the
        // next load rebuilds it from the tables. Anything meant to outlive a save has to be
        // expressed as something the file carries -- a bioma, a resource, an improvement.
        saveMap(savename);
        loadMap(savename);
        if (map.peek(RIVER_LAT,RIVER_LON).getResourceProductionRate(FOOD) != EXPECT_RIVER_FOOD)
        { fail("After a load a tile's rates come from the tables again -- a hand-set rate is runtime-only by design."); return 0; }
        if (!verify("after reloading over a hand-set rate")) return 0;
    }

    haspassed = true;
    return 0;
}

std::string TestCase_066::title()
{
    return std::string("Map save/load no longer stores tile production rates: tiles keep their own (so one tile can be singled out), but assignProductionRates() fills them from the productionrates tables on a generated world AND inside loadMap(), so an improved tile survives repeated save/load cycles unchanged instead of re-applying its improvement bonus every time (an irrigated river tile used to go 2 -> 4 -> 8 -> 16).");
}

bool TestCase_066::done()   { return isdone; }
bool TestCase_066::passed() { return haspassed; }
std::string TestCase_066::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_066();
}
