//  TestCase_062.cpp
//  bunmei
//
//  Created by Claude on 09/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cmath>
#include <vector>

#include "../map.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../codes.h"
#include "../dee.h"
#include "../technologies.h"
#include "../savegame.h"

#include "testcase_062.h"

// @Task: savegames must carry the Dependency Evaluation Engine registry (world, faction AND
// city scope) and the tech graph.
//
// What is persisted, and why only this:
//   * The DEE registry verbatim -- a contextId already encodes its scope, so one map covers
//     all three levels. Restoring it REPLACES whatever setup registered, which incidentally
//     fixes city-level perks: loadCities() rebuilds a city's buildings but never re-registers
//     their perk codes (only bunmei.cpp does, when one is actually built), so before this a
//     Granary's HALF_POPULATION_CODE vanished across a save/load.
//   * Per faction: the research target, the banked SCIENCE, and the id+science of every
//     DISCOVERED technology. NOT the graph itself (buildDefaultTechGraph rebuilds nodes,
//     edges, README weights and depth biases) and NOT Frontier/Next (pure functions of the
//     discovered flags, recomputed by TechGraph::rebuildFrontier).
//
// savegame()/loadWorldModelling() live in files the testcase build does not link, so this
// drives the four serialization functions directly through a real file.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Tiles tiles;
extern DependencyEvaluationEngine dee;
extern TechTree techtree;

extern float mapzoom;
extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1
#define SAVE_PATH    "tmp/testcase_062.sav"

// savegame.cpp's writers are static, so the test drives the same blocks through the public
// loaders by writing an equivalent stream itself. Keeping these in lockstep with
// saveDependencies()/saveTechnologies() is the point of the round-trip assertions below.
static void writeDependencies(std::ofstream& out)
{
    const auto& registry = dee.getRegistry();
    size_t context_count = registry.size();
    out.write(reinterpret_cast<const char*>(&context_count), sizeof(context_count));
    for (const auto& entry : registry)
    {
        int contextId = entry.first;
        size_t code_count = entry.second.size();
        out.write(reinterpret_cast<const char*>(&contextId), sizeof(contextId));
        out.write(reinterpret_cast<const char*>(&code_count), sizeof(code_count));
        for (int codeId : entry.second)
            out.write(reinterpret_cast<const char*>(&codeId), sizeof(codeId));
    }
}

static void writeTechnologies(std::ofstream& out)
{
    size_t faction_count = techtree.factionCount();
    out.write(reinterpret_cast<const char*>(&faction_count), sizeof(faction_count));
    for (size_t f = 0; f < faction_count; ++f)
    {
        const TechGraph& g = techtree.graph((int)f);
        int target  = techtree.getResearchTarget((int)f);
        int pending = techtree.getPendingScience((int)f);
        out.write(reinterpret_cast<const char*>(&target), sizeof(target));
        out.write(reinterpret_cast<const char*>(&pending), sizeof(pending));

        std::vector<int> discovered;
        for (int id : g.getTechIds())
            if (g.isDiscovered(id)) discovered.push_back(id);

        size_t discovered_count = discovered.size();
        out.write(reinterpret_cast<const char*>(&discovered_count), sizeof(discovered_count));
        for (int id : discovered)
        {
            int science = g.getTech(id)->science;
            out.write(reinterpret_cast<const char*>(&id), sizeof(id));
            out.write(reinterpret_cast<const char*>(&science), sizeof(science));
        }
    }
}

TestCase_062::TestCase_062() {}
TestCase_062::~TestCase_062() {}

int TestCase_062::number() { return 62; }

void TestCase_062::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
            map.set(lat,lon).setVisible(0);
        }

    Faction *a = new Faction();
    a->id = 0; strcpy(a->name,"Vikings");
    a->red = 255; a->green = 0; a->blue = 0; a->autoPlayer = false;
    factions.push_back(a);

    Faction *b = new Faction();
    b->id = 1; strcpy(b->name,"Romans");
    b->red = 0; b->green = 0; b->blue = 255; b->autoPlayer = true;
    factions.push_back(b);

    City *city = new City(&map, 0, getNextCityId(), 3, 3);
    city->setName("Kattegate");
    cities[city->id] = city;

    citynames[0] = std::queue<std::string>();
    citynames[1] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
}

int TestCase_062::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    int cityid = cities.begin()->first;

    // ---- build a distinctive state: all three DEE scopes plus real tech progress ---------
    initTechnologies(techtree, (int)factions.size(), dee);

    dee.regDep(worldContext(), 0x42);                       // world scope
    dee.regDep(factionContext(1), 0x77);                     // faction scope, faction 1 only
                                                            // (a sentinel, not a TECH_* code:
                                                            // faction 0 legitimately discovers
                                                            // several technologies below, so a
                                                            // real code could not prove scoping)
    dee.regDep(cityContext(cityid), HALF_POPULATION_CODE);  // city scope
    dee.regDep(cityContext(cityid), STORAGE_EXPANSION_1);

    // Faction 0 researches for real, so its graph carries discovered nodes AND accumulated
    // science; faction 1 is left at the root with a banked, unspent year.
    techtree.setResearchTarget(0, TECH_ROOT);
    techtree.advance(0, 400, dee);                          // fires Language's children
    std::vector<int> frontier0 = techtree.graph(0).getFrontierOrdered();
    if (frontier0.empty())
    { fail("Setup: faction 0 should have a Frontier after researching."); return 0; }
    techtree.setResearchTarget(0, frontier0[0]);
    techtree.advance(0, 150, dee);

    techtree.advance(1, 60, dee);                           // no target -> banked, not spent

    if (!techtree.graph(0).isDiscovered(TECH_ALPHABET))
    { fail("Setup: 400 SCIENCE into Language should have discovered Alphabet."); return 0; }
    if (techtree.getPendingScience(1) != 60)
    { fail("Setup: faction 1 should have 60 SCIENCE banked and unspent."); return 0; }

    // Snapshot everything the round trip has to reproduce.
    int  savedTarget0   = techtree.getResearchTarget(0);
    int  savedPending0  = techtree.getPendingScience(0);
    int  savedPending1  = techtree.getPendingScience(1);
    std::vector<int> savedDiscovered, savedScience;
    for (int id : techtree.graph(0).getTechIds())
        if (techtree.graph(0).isDiscovered(id))
        {
            savedDiscovered.push_back(id);
            savedScience.push_back(techtree.graph(0).getTech(id)->science);
        }
    size_t savedContexts = dee.getRegistry().size();

    // ---- write ---------------------------------------------------------------------------
    {
        std::ofstream out(SAVE_PATH, std::ios::binary);
        if (!out) { fail("Could not open " SAVE_PATH " for writing."); return 0; }
        writeDependencies(out);
        writeTechnologies(out);
        out.close();
    }

    // ---- wipe: this is the state a fresh load starts from ---------------------------------
    dee.clear();
    initTechnologies(techtree, (int)factions.size(), dee);

    if (dee.verifyDep(worldContext(), 0x42) ||
        dee.verifyDep(cityContext(cityid), HALF_POPULATION_CODE) ||
        techtree.graph(0).isDiscovered(TECH_ALPHABET))
    { fail("Setup: the wipe should have removed the saved state before reloading."); return 0; }

    // ---- read back ------------------------------------------------------------------------
    {
        std::ifstream in(SAVE_PATH, std::ios::binary);
        if (!in) { fail("Could not open " SAVE_PATH " for reading."); return 0; }
        loadDependencies(in);
        loadTechnologies(in);
        in.close();
    }

    // ---- 1) all three DEE scopes came back ------------------------------------------------
    if (!dee.verifyDep(worldContext(), 0x42))
    { fail("A WORLD-scope dependency did not survive the save/load."); return 0; }
    if (!dee.verifyDep(factionContext(1), 0x77))
    { fail("A FACTION-scope dependency did not survive the save/load."); return 0; }
    // Faction 0 discovered its own technologies, so those codes must be back too.
    if (!dee.verifyDep(factionContext(0), TECH_ALPHABET))
    { fail("A technology faction 0 discovered did not re-register its code on load."); return 0; }
    if (!dee.verifyDep(cityContext(cityid), HALF_POPULATION_CODE) ||
        !dee.verifyDep(cityContext(cityid), STORAGE_EXPANSION_1))
    { fail("A CITY-scope dependency did not survive the save/load (both codes of that city)."); return 0; }
    if (dee.getRegistry().size() != savedContexts)
    { fail("The restored registry has a different number of contexts than the saved one."); return 0; }
    // Scopes must stay separate -- faction 1's code must not have leaked onto faction 0.
    if (dee.verifyDep(factionContext(0), 0x77))
    { fail("A faction-scope code was restored against the WRONG faction."); return 0; }
    if (dee.verifyDep(factionContext(1), TECH_ALPHABET))
    { fail("Faction 1 never discovered Alphabet -- its code must not be registered for it."); return 0; }

    // ---- 2) tech progress came back --------------------------------------------------------
    std::vector<int> gotDiscovered, gotScience;
    for (int id : techtree.graph(0).getTechIds())
        if (techtree.graph(0).isDiscovered(id))
        {
            gotDiscovered.push_back(id);
            gotScience.push_back(techtree.graph(0).getTech(id)->science);
        }

    if (gotDiscovered != savedDiscovered)
    {
        char buf[200];
        snprintf(buf,sizeof(buf),"Faction 0 came back with %d discovered technologies, saved %d.",
                 (int)gotDiscovered.size(), (int)savedDiscovered.size());
        fail(buf); return 0;
    }
    if (gotScience != savedScience)
    { fail("The SCIENCE accumulated in each discovered technology did not survive the save/load."); return 0; }
    if (techtree.getResearchTarget(0) != savedTarget0)
    { fail("The research target did not survive the save/load."); return 0; }
    if (techtree.getPendingScience(0) != savedPending0 ||
        techtree.getPendingScience(1) != savedPending1)
    { fail("Banked (unspent) SCIENCE did not survive the save/load."); return 0; }
    if (techtree.graph(1).isDiscovered(TECH_ALPHABET))
    { fail("Faction 1 researched nothing -- it must not come back with faction 0's discoveries."); return 0; }

    // ---- 3) the DERIVED state was rebuilt, not restored -------------------------------------
    // Frontier/Next are never written to the file; rebuildFrontier() has to reproduce them
    // from the discovered flags alone.
    if (techtree.graph(0).getFrontier().empty())
    { fail("The Frontier was not rebuilt after loading -- research could not continue."); return 0; }
    if (techtree.graph(0).getFrontier().count(TECH_ROOT) != 0)
    { fail("Language's children are all known, so it must NOT be back in the rebuilt Frontier."); return 0; }
    if (techtree.graph(0).getNext().empty())
    { fail("Next was not rebuilt after loading -- nothing could ever fire again."); return 0; }

    // And the graph itself (weights, biases) is the rebuilt default, not something stale.
    if (fabs(techtree.graph(0).getWeight(TECH_LANGUAGE, TECH_ARCHERY) - 0.8f*TECH_DEFAULT_WEIGHT) > 0.0001f)
    { fail("The restored graph does not carry README.md's weights -- it should be rebuilt, not loaded."); return 0; }

    // ---- 4) research still works after the load ---------------------------------------------
    std::vector<int> more = techtree.advance(0, 2000, dee);
    if (more.empty())
    { fail("A loaded faction should be able to keep researching."); return 0; }

    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_062::title()
{
    return std::string("Savegames carry the DEE registry at world/faction/city scope and each faction's tech progress (discovered technologies + their SCIENCE, research target, banked science); the graph and the Frontier/Next sets are rebuilt on load rather than stored.");
}

bool TestCase_062::done()   { return isdone; }
bool TestCase_062::passed() { return haspassed; }
std::string TestCase_062::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_062();
}
