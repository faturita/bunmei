//  TestCase_056.cpp
//  bunmei
//
//  Created by Claude on 07/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <vector>
#include <unordered_map>

#include "../map.h"
#include "../units/Unit.h"
#include "../units/Warrior.h"
#include "../units/Swordman.h"
#include "../units/Settler.h"
#include "../buildings/Depot.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"

#include "testcase_056.h"

// @Task: BuildableFactory's old `int cost(int r_id)` (one flat number, only ever queried for
// SHIELDS) was replaced by a two-part recipe interface:
//   std::vector<int>       getRequiredResources()            -- every id the recipe may touch
//   std::vector<Resource*> fullfillment(available)           -- the exact (id,amount) list to
//                                                              deduct, or empty == can't build
// This drives the production step in bunmei.cpp:endOfYear() (and its simulate.cpp mirror).
// This test checks the three shapes now in the codebase:
//   * Warrior  -- the basic single-resource recipe (40 SHIELDS)
//   * Swordman -- SHIELDS + (iron OR copper): an alternative
//   * Depot    -- SHIELDS AND tools: a conjunction
// and then replays endOfYear()'s consume loop against a real City to prove the wiring.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Tiles tiles;

extern float mapzoom;
extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_056::TestCase_056() {}
TestCase_056::~TestCase_056() {}

int TestCase_056::number() { return 56; }

void TestCase_056::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon) = mapcell(LAND);
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon).setVisible(0);

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255; faction->green = 0; faction->blue = 0;
    faction->autoPlayer = false;
    factions.push_back(faction);

    City *city = new City(&map, 0, getNextCityId(), 3, 3);
    city->setName("Kattegate");
    city->foundedyear = -4000;
    cities[city->id] = city;
    cityid = city->id;

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
}

// endOfYear() builds availableResources as { id -> new Resource{id, city stock} } for exactly
// the ids the factory asks for; do the same here so we exercise the real call shape.
static std::vector<Resource*> runFulfillment(BuildableFactory* bf, std::unordered_map<int,int> stock)
{
    std::unordered_map<int, Resource*> available;
    for (int id : bf->getRequiredResources())
        available[id] = new Resource{id, stock.count(id) ? stock[id] : 0};

    std::vector<Resource*> consumed = bf->fullfillment(available);

    for (auto& kv : available) delete kv.second;
    return consumed;
}

static bool consumes(const std::vector<Resource*>& v, int id, int amount)
{
    for (Resource* r : v)
        if (r->id == id && r->amount == amount) return true;
    return false;
}

int TestCase_056::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    // ---- 1) Warrior: 40 SHIELDS, nothing else ---------------------------------------------
    {
        WarriorFactory wf;
        std::vector<int> req = wf.getRequiredResources();
        if (req.size() != 1 || req[0] != SHIELDS)
        { fail("WarriorFactory::getRequiredResources() should be exactly { SHIELDS }."); return 0; }

        std::vector<Resource*> ok = runFulfillment(&wf, {{SHIELDS,40}});
        if (ok.size() != 1 || !consumes(ok, SHIELDS, 40))
        { fail("WarriorFactory::fullfillment() with 40 shields should consume exactly {SHIELDS:40}."); return 0; }

        std::vector<Resource*> broke = runFulfillment(&wf, {{SHIELDS,39}});
        if (!broke.empty())
        { fail("WarriorFactory::fullfillment() with 39 shields should be empty (cannot build)."); return 0; }
    }

    // ---- 2) Swordman: 40 SHIELDS + (25 iron OR 25 copper) --------------------------------
    {
        SwordmanFactory sf;
        std::vector<int> req = sf.getRequiredResources();
        bool hasS=false, hasI=false, hasC=false;
        for (int id : req) { hasS |= (id==SHIELDS); hasI |= (id==iron); hasC |= (id==copper); }
        if (!hasS || !hasI || !hasC)
        { fail("SwordmanFactory::getRequiredResources() must list SHIELDS, iron and copper."); return 0; }

        // iron branch (iron present -> iron is taken, copper untouched)
        std::vector<Resource*> ironpath = runFulfillment(&sf, {{SHIELDS,40},{iron,25},{copper,25}});
        if (!(ironpath.size()==2 && consumes(ironpath,SHIELDS,40) && consumes(ironpath,iron,25)))
        { fail("SwordmanFactory::fullfillment() with iron available should consume {SHIELDS:40, iron:25}."); return 0; }

        // copper fallback (no iron)
        std::vector<Resource*> copperpath = runFulfillment(&sf, {{SHIELDS,40},{iron,0},{copper,25}});
        if (!(copperpath.size()==2 && consumes(copperpath,SHIELDS,40) && consumes(copperpath,copper,25)))
        { fail("SwordmanFactory::fullfillment() with no iron but copper should consume {SHIELDS:40, copper:25}."); return 0; }

        // neither metal
        std::vector<Resource*> nometal = runFulfillment(&sf, {{SHIELDS,40},{iron,10},{copper,10}});
        if (!nometal.empty())
        { fail("SwordmanFactory::fullfillment() with neither 25 iron nor 25 copper should be empty."); return 0; }

        // shields short-circuits even with metal
        std::vector<Resource*> noshields = runFulfillment(&sf, {{SHIELDS,39},{iron,25},{copper,25}});
        if (!noshields.empty())
        { fail("SwordmanFactory::fullfillment() with < 40 shields should be empty regardless of metal."); return 0; }
    }

    // ---- 2b) Settler: 40 SHIELDS AND 25 FOOD (a conjunction across resource classes) ----
    {
        SettlerFactory sf;
        std::vector<int> req = sf.getRequiredResources();
        bool hasS=false, hasF=false;
        for (int id : req) { hasS |= (id==SHIELDS); hasF |= (id==FOOD); }
        if (!hasS || !hasF)
        { fail("SettlerFactory::getRequiredResources() must list SHIELDS and FOOD."); return 0; }

        std::vector<Resource*> ok = runFulfillment(&sf, {{SHIELDS,40},{FOOD,25}});
        if (!(ok.size()==2 && consumes(ok,SHIELDS,40) && consumes(ok,FOOD,25)))
        { fail("SettlerFactory::fullfillment() with 40 shields + 25 food should consume {SHIELDS:40, FOOD:25}."); return 0; }

        std::vector<Resource*> nofood = runFulfillment(&sf, {{SHIELDS,40},{FOOD,24}});
        if (!nofood.empty())
        { fail("SettlerFactory::fullfillment() with only 24 food should be empty (needs BOTH)."); return 0; }
    }

    // ---- 3) Depot: 100 SHIELDS AND 100 tools (a conjunction) ----------------------------
    {
        DepotFactory df;
        std::vector<int> req = df.getRequiredResources();
        bool hasS=false, hasT=false;
        for (int id : req) { hasS |= (id==SHIELDS); hasT |= (id==tools); }
        if (!hasS || !hasT)
        { fail("DepotFactory::getRequiredResources() must list SHIELDS and tools."); return 0; }

        std::vector<Resource*> both = runFulfillment(&df, {{SHIELDS,100},{tools,100}});
        if (!(both.size()==2 && consumes(both,SHIELDS,100) && consumes(both,tools,100)))
        { fail("DepotFactory::fullfillment() with 100/100 should consume {SHIELDS:100, tools:100}."); return 0; }

        std::vector<Resource*> shorttools = runFulfillment(&df, {{SHIELDS,100},{tools,99}});
        if (!shorttools.empty())
        { fail("DepotFactory::fullfillment() with only 99 tools should be empty (needs BOTH)."); return 0; }
    }

    // ---- 4) end-to-end: the endOfYear() consume loop against a real City -----------------
    {
        City* city = cities[cityid];
        city->resources[SHIELDS] = 40;
        city->resources[copper]  = 30;   // enough copper, NO iron -> Swordman takes the copper path
        city->resources[iron]    = 0;

        BuildableFactory* bf = new SwordmanFactory();
        while (!city->productionQueue.empty()) city->productionQueue.pop();
        city->productionQueue.push(bf);

        // --- verbatim shape of bunmei.cpp:endOfYear() ---
        std::vector<int> requiredResources = bf->getRequiredResources();
        std::unordered_map<int, Resource*> availableResources;
        for (int r_id : requiredResources)
            availableResources[r_id] = new Resource{r_id, city->resources[r_id]};
        std::vector<Resource*> consumedResources = bf->fullfillment(availableResources);
        int unitsBefore = (int)units.size();
        if (consumedResources.size() > 0)
        {
            for (Resource* r : consumedResources)
                city->resources[r->id] -= r->amount;
            city->productionQueue.pop();
            Buildable* b = bf->create();
            if (b->getType() == BuildableType::UNIT)
            {
                Unit* u = (Unit*)b;
                u->id = getNextUnitId();
                u->faction = city->faction;
                units[u->id] = u;
            }
        }
        for (auto& kv : availableResources) delete kv.second;
        for (Resource* r : consumedResources) delete r;

        if (city->resources[SHIELDS] != 0)
        { fail("endOfYear consume loop: SHIELDS should have dropped 40 -> 0."); return 0; }
        if (city->resources[copper] != 5)
        { fail("endOfYear consume loop: copper should have dropped 30 -> 5 (25 consumed)."); return 0; }
        if (city->resources[iron] != 0)
        { fail("endOfYear consume loop: iron must be untouched (the recipe took the copper branch)."); return 0; }
        if ((int)units.size() != unitsBefore + 1)
        { fail("endOfYear consume loop: a Swordman should have been created."); return 0; }
        if (!city->productionQueue.empty())
        { fail("endOfYear consume loop: the production queue should have been popped."); return 0; }
    }

    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_056::title()
{
    return std::string("BuildableFactory recipe interface: getRequiredResources() + fullfillment() replace cost() -- single (Warrior), alternative (Swordman iron|copper) and conjunction (Depot shields+tools) recipes, plus the endOfYear() consume loop.");
}

bool TestCase_056::done()   { return isdone; }
bool TestCase_056::passed() { return haspassed; }
std::string TestCase_056::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_056();
}
