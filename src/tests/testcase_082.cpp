//  TestCase_082.cpp
//  bunmei
//
//  Created by faturita on 29/09/2026
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
#include <queue>

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
#include "../savegame.h"
#include "../dee.h"
#include "../technologies.h"

#include "testcase_082.h"

// Savegames store the faction list as it was at the time of the save. Ids are not contiguous
// (lost factions leave gaps) and not tied to a civilization (new games are shuffled), and every
// other block names factions by id, so the load has to give back exactly those ids, in the same
// turn order, with the same civilizations, players and rates -- and keep handing out ids after
// the highest one ever given, not after the highest one still alive.
//
// Driven through the real format: savegame() to a file, then readSaveGame(), the year and
// loadFactions()/loadCities() in the order gamekernel.cpp's loadWorldModelling() uses.

extern std::unordered_map<int, std::string> tiles;
extern std::unordered_map<int, Improvement*> improvements;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, City*> cities;
extern Factions factions;
extern DependencyEvaluationEngine dee;
extern TechTree techtree;
extern Map map;
extern float mapzoom;

extern Coordinator coordinator;
extern int year;

#define TEST_MAPSIZE 1

TestCase_082::TestCase_082() {}
TestCase_082::~TestCase_082() {}

int TestCase_082::number() { return 82; }

void TestCase_082::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initImprovements(improvements);
    initNaming(citynames);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
            map.set(lat,lon).bioma = GRASSLAND;
            map.set(lat,lon).setVisible(0);
        }

    // A shuffled game, ids 0..4: Mongols, Greeks, Aztec, Egyptians, Zulus. Then the Greeks
    // (id 1, a gap in the middle) and the Zulus (id 4, the highest id) are lost.
    const int rows[] = { 7, 2, 12, 4, 9 };
    for (int row : rows)
    {
        Faction *f = createFaction(row);
        f->autoPlayer = true;
        factions.push_back(f);
    }
    factions.erase(1);
    factions.erase(4);

    // The Aztec are the human player, with their own rates.
    factions[2]->autoPlayer = false;
    factions[2]->rates[0] = 0.1f; factions[2]->rates[1] = 0.2f;
    factions[2]->rates[2] = 0.3f; factions[2]->rates[3] = 0.4f;

    initTechnologies(techtree, factions.nextId(), dee);

    // An Egyptian city: faction id 3, past the gap.
    City *city = new City(&map, 3, getNextCityId(), 0, 0);
    city->setName(citynames[4].front().c_str());
    citynames[4].pop();
    city->setCityPop(1);
    city->setCapitalCity();
    cities[city->id] = city;

    char namebuf[64];
    snprintf(namebuf, sizeof(namebuf), "testcase082_%d", (int)((time(nullptr) ^ getpid()) & 0xffffff));
    savename = std::string("saves/") + namebuf;
    savegame(savename.c_str());

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
}

int TestCase_082::check(int year)
{
    ticks++;

    if (isdone) return 0;
    if (ticks != 5) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    isdone = true;
    char buf[256];

    // ---- wipe: a different faction list and no cities, so only the file can restore them ----
    factions.clear();
    for (int row = 0; row < 6; row++)
        factions.push_back(createFaction(row));
    for (auto& [k, c] : cities)
        delete c;
    cities.clear();
    initNaming(citynames);

    loadMap(savename + ".map");

    std::string savedata;
    SaveGameInfo saveinfo;
    if (!readSaveGame(savename.c_str(), savedata, saveinfo))
    { fail("readSaveGame() rejected the file this test just wrote."); return 0; }
    std::istringstream in(savedata, std::ios::binary);

    int loadedYear = 0;
    in.read(reinterpret_cast<char*>(&loadedYear), sizeof(loadedYear));
    loadFactions(in);
    loadCities(in);

    // ---- the list, its ids and its turn order ------------------------------------------------
    if (factions.size() != 3 || !factions.has(0) || factions.has(1) || !factions.has(2) ||
        !factions.has(3) || factions.has(4) || factions.has(5))
    {
        sprintf(buf, "loaded %zu factions; expected exactly ids 0, 2, 3 (1 and 4 were lost).", factions.size());
        fail(buf); return 0;
    }
    if (factions.first() != 0 || factions.next(0) != 2 || factions.next(2) != 3 || factions.next(3) != -1)
    { fail("the turn order is not the saved one (0, 2, 3)."); return 0; }
    if (factions.nextId() != 5)
    {
        sprintf(buf, "next id is %d after the load, expected 5: the lost Zulus' id 4 would be handed out again.", factions.nextId());
        fail(buf); return 0;
    }

    // ---- each faction is the civilization it was ---------------------------------------------
    struct { int id; int row; const char* name; } expected[] = {
        { 0, 7, "Mongols" }, { 2, 12, "Aztec" }, { 3, 4, "Egyptians" } };
    for (auto& e : expected)
    {
        Faction *f = factions[e.id];
        if (f->definition != e.row || strcmp(f->name, e.name) != 0)
        {
            sprintf(buf, "faction %d came back as %s (row %d), expected %s (row %d).", e.id, f->name, f->definition, e.name, e.row);
            fail(buf); return 0;
        }
        Faction *reference = createFaction(e.row);
        bool same = f->red == reference->red && f->green == reference->green &&
                    f->blue == reference->blue && f->song == reference->song;
        delete reference;
        if (!same)
        {
            sprintf(buf, "faction %d (%s) did not get its colour or its song back.", e.id, e.name);
            fail(buf); return 0;
        }
    }

    // ---- players and rates ---------------------------------------------------------------------
    if (factions[2]->autoPlayer || !factions[0]->autoPlayer || !factions[3]->autoPlayer)
    { fail("the Aztec were the human player and the others the computer; the load changed that."); return 0; }
    if (factions[2]->rates[0] != 0.1f || factions[2]->rates[1] != 0.2f ||
        factions[2]->rates[2] != 0.3f || factions[2]->rates[3] != 0.4f)
    { fail("the Aztec's fundamental rates were not restored."); return 0; }

    // ---- the rest of the save still names the right faction -------------------------------------
    if (cities.size() != 1 || cities.begin()->second->faction != 3 ||
        strcmp(factions[cities.begin()->second->faction]->name, "Egyptians") != 0)
    { fail("the Egyptian city did not come back to faction 3, the Egyptians."); return 0; }

    haspassed = true;
    return 0;
}

std::string TestCase_082::title()
{
    return std::string("Savegame: the faction list round-trips with its ids, gaps, turn order, next id, civilizations, players and rates.");
}

bool TestCase_082::done() { return isdone; }
bool TestCase_082::passed() { return haspassed; }
std::string TestCase_082::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_082();
}
