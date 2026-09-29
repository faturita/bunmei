//  TestCase_081.cpp
//  bunmei
//
//  Created by faturita on 29/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <set>

#include "../map.h"
#include "../units/Unit.h"
#include "../units/Settler.h"
#include "../City.h"
#include "../Faction.h"
#include "../coordinator.h"
#include "../commandorder.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../diplomacy.h"
#include "../dee.h"
#include "../technologies.h"

#include "testcase_081.h"

// A new game shuffles which civilizations play and their turn order, and -faction names the
// player's civilization: findFactionDefinition() and pickStartingCivilizations(), which
// gamekernel.cpp's initFactions() is built on (gamekernel.cpp is not linked here). Once a
// faction id is no longer its table row, a founded city must take its name from the
// CIVILIZATION's pool (Faction::definition), not from the pool at the faction's id.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern Factions factions;
extern Tiles tiles;
extern MovementCost movementcosts;
extern DependencyEvaluationEngine dee;
extern TechTree techtree;
extern DiplomacyTable diplomacy;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_081::TestCase_081()
{

}

TestCase_081::~TestCase_081()
{

}

int TestCase_081::number()
{
    return 81;
}

static int addSettler(int faction, int lat, int lon)
{
    Settler *s = new Settler();
    s->id = getNextUnitId(); s->faction = faction;
    s->latitude = lat; s->longitude = lon;
    s->availablemoves = s->getUnitMoves();
    units[s->id] = s;
    map.set(lat,lon).setOwnedBy(faction);
    return s->id;
}

void TestCase_081::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initMovementCosts(movementcosts);
    initNaming(citynames);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
            map.set(lat,lon).bioma = GRASSLAND;
            map.set(lat,lon).setVisible(0);
        }

    // A shuffled start: faction 0 is the Babylonians (row 5), faction 1 the Greeks (row 2).
    Faction *babylonians = createFaction(5);
    babylonians->autoPlayer = true;          // research is rolled, no selector dialog opens
    factions.push_back(babylonians);
    Faction *greeks = createFaction(2);
    greeks->autoPlayer = true;
    factions.push_back(greeks);

    initDiplomacy(diplomacy, (int)factions.size());
    initTechnologies(techtree, (int)factions.size(), dee);

    settlerA = addSettler(0, 0, 0);
    settlerB = addSettler(1, 10, 10);

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

}

int TestCase_081::check(int year)
{

    ticks++;

    if (isdone)
        return 0;

    if (ticks < 3)
        return 0;

    isdone = true;
    haspassed = false;
    char buf[256];

    const int n = numberOfFactionDefinitions();

    // ---- -faction <name> -------------------------------------------------------------------
    if (findFactionDefinition("Romans") != 1 || findFactionDefinition("romans") != 1 ||
        findFactionDefinition("Spanish") != n - 1 || findFactionDefinition("Atlanteans") != -1 ||
        findFactionDefinition(nullptr) != -1)
    {
        message = std::string("findFactionDefinition: Romans/romans must be row 1, Spanish the last row, unknown names -1.");
        return 0;
    }
    if (strcmp(factionDefinitionName(findFactionDefinition("zulus")), "Zulus") != 0)
    {
        message = std::string("factionDefinitionName does not give back the table's name.");
        return 0;
    }

    // ---- which civilizations start -----------------------------------------------------------
    // Always count distinct valid rows with the player's among them, and over many
    // games both the set and the order change.
    std::set<std::vector<int>> seen;
    std::set<int> firsts;
    for (int game = 0; game < 200; game++)
    {
        std::vector<int> rows = pickStartingCivilizations(4, 7);
        std::set<int> distinct(rows.begin(), rows.end());
        if (rows.size() != 4 || distinct.size() != 4 || *distinct.begin() < 0 || *distinct.rbegin() >= n)
        {
            message = std::string("a shuffled start is not 4 distinct table rows.");
            return 0;
        }
        if (distinct.count(7) == 0)
        {
            message = std::string("a shuffled start left out the player's civilization (-faction Mongols).");
            return 0;
        }
        seen.insert(rows);
        firsts.insert(rows[0]);
    }
    if (seen.size() < 100 || firsts.size() < n / 2)
    {
        sprintf(buf, "200 shuffled starts gave only %zu different line-ups and %zu different first players.",
                seen.size(), firsts.size());
        message = std::string(buf);
        return 0;
    }
    std::vector<int> all = pickStartingCivilizations(n, -1);
    if ((int)std::set<int>(all.begin(), all.end()).size() != n)
    {
        message = std::string("a shuffled start of every civilization does not hold every row once.");
        return 0;
    }

    // ---- city names follow the civilization, not the faction id -------------------------------
    std::string babylonian = citynames[5].front();
    std::string greek = citynames[2].front();

    CommandOrder co;
    co.command = Command::BuildCityOrder;
    co.parameters.spawnid = settlerA;
    co.parameters.factionid = 0;
    coordinator.push(co);
    co.parameters.spawnid = settlerB;
    co.parameters.factionid = 1;
    coordinator.push(co);
    processCommandOrders();

    std::string nameA, nameB;
    for (auto& [k, c] : cities)
    {
        if (c->faction == 0) nameA = c->name;
        if (c->faction == 1) nameB = c->name;
    }
    if (nameA != babylonian || nameB != greek)
    {
        sprintf(buf, "cities were named '%s' and '%s', expected the Babylonian '%s' and the Greek '%s'.",
                nameA.c_str(), nameB.c_str(), babylonian.c_str(), greek.c_str());
        message = std::string(buf);
        return 0;
    }

    haspassed = true;

    return 0;
}
std::string TestCase_081::title()
{
    return std::string("Factions: -faction by name, shuffled starting civilizations, city names follow the civilization.");

}

bool TestCase_081::done()
{
    return isdone;
}
bool TestCase_081::passed()
{
    return haspassed;
}
std::string TestCase_081::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_081();
}
