//  TestCase_079.cpp
//  bunmei
//
//  Created by faturita on 28/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../map.h"
#include "../units/Unit.h"
#include "../units/Warrior.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../dee.h"
#include "../technologies.h"

#include "testcase_079.h"

// Culture (README.md Culture section): every year each city's CULTURE Q heats the LAND around it,
// Q / (CULTURE_K * (d+1)^2), d = the unit-movement path; a faction with more than half of a tile's
// heat (over CULTURE_THRESHOLD) takes the tile like an army.  Driven through spreadCulture() on a
// hand-built grassland map, with every expectation worked out from the formula at the constants
// the test pins (K 1, threshold 0.25), plus one real endOfYear() to prove it is wired in.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern Factions factions;
extern Tiles tiles;
extern MovementCost movementcosts;
extern DependencyEvaluationEngine dee;
extern TechTree techtree;
extern void endOfYear();

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_079::TestCase_079()
{

}

TestCase_079::~TestCase_079()
{

}

int TestCase_079::number()
{
    return 79;
}

static int addCity(int faction, int lat, int lon, const char* name)
{
    City *city = new City(&map, faction, getNextCityId(), lat, lon);
    city->setName(name);
    city->foundedyear = -4000;
    cities[city->id] = city;
    return city->id;
}

void TestCase_079::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initMovementCosts(movementcosts);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
            map.set(lat,lon).bioma = GRASSLAND;
            map.set(lat,lon).setVisible(0);
        }

    // A mountain ridge 2 east of the ridge city, and a sea strait 2 east of the strait city,
    // both 11 tiles long so going around costs more than the city's reach.
    for (int lat=5;lat<=15;lat++)
        map.set(lat,-18).bioma = MOUNTAINS;
    for (int lat=-15;lat<=-5;lat++)
        map.set(lat,-18) = mapcell(OCEAN);

    for (int f=0;f<2;f++)
    {
        Faction *faction = new Faction();
        faction->id = f;
        strcpy(faction->name, f == 0 ? "Vikings" : "Romans");
        faction->red = 255; faction->green = 0; faction->blue = 0;
        faction->autoPlayer = true;          // research is rolled, no selector dialog opens
        factions.push_back(faction);
        citynames[f] = std::queue<std::string>();
    }

    initTechnologies(techtree, (int)factions.size(), dee);     // endOfYear() researches for every faction

    ringid   = addCity(0,   0, -20, "Ring");          // Q 4: reach 3, owns rings 1-2
    ridgeid  = addCity(0,  10, -20, "Ridge");         // Q 9: reach 5, mountains to the east
    strait   = addCity(0, -10, -20, "Strait");        // Q 9: reach 5, sea to the east
    pairA    = addCity(0,   0,   0, "PairA");         // Q 1 each, 2 apart: only together they own
    pairB    = addCity(0,   0,   2, "PairB");         //   the tiles between them
    weakid   = addCity(0,   0,  15, "Weak");          // Q 4 against
    strongid = addCity(1,   0,  21, "Strong");        // Q 16, 6 tiles east
    outpost  = addCity(0,   0,  24, "Outpost");       // Q 0, deep in Strong's culture

    // A Roman warrior stands on a tile the Vikings' culture wins: it must keep it.
    Warrior *w = new Warrior();
    w->id = getNextUnitId(); w->faction = 1;
    w->latitude = 1; w->longitude = 16;
    w->availablemoves = w->getUnitMoves();
    units[w->id] = w;
    map.set(1,16).setOwnedBy(1);
    guardid = w->id;

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

}

int TestCase_079::check(int year)
{

    ticks++;

    if (isdone)
        return 0;

    if (ticks < 3)
        return 0;

    isdone = true;
    haspassed = false;
    char buf[256];

    if (CULTURE_K != 1.0f || CULTURE_THRESHOLD != 0.25f || CULTURE_ALLEGIANCE != 0.5f)
    {
        message = std::string("the expectations below are worked out for CULTURE_K 1, CULTURE_THRESHOLD 0.25, CULTURE_ALLEGIANCE 0.5.");
        return 0;
    }

    auto owner = [&](int lat, int lon) { return map.peek(lat, lon).getOwnedBy(); };
    auto expect = [&](int lat, int lon, int f, const char* why) -> bool
    {
        if (owner(lat, lon) == f) return true;
        sprintf(buf, "(%d,%d): owner %d, expected %d -- %s", lat, lon, owner(lat, lon), f, why);
        message = std::string(buf);
        return false;
    };

    cities[ringid]->resources[CULTURE]   = 4;
    cities[ridgeid]->resources[CULTURE]  = 9;
    cities[strait]->resources[CULTURE]   = 9;
    cities[pairA]->resources[CULTURE]    = 1;
    cities[pairB]->resources[CULTURE]    = 1;
    cities[weakid]->resources[CULTURE]   = 4;
    cities[strongid]->resources[CULTURE] = 16;

    // A worked tile of the weak city that the strong culture wins (see below).
    cities[weakid]->assignTile(coordinate(0, 2));
    if (!cities[weakid]->workingOn(0, 2))
    {
        message = std::string("setup: Weak could not work (0,17).");
        return 0;
    }

    spreadCulture();

    // Spent: CULTURE is not a stockpile.
    for (auto& [k, c] : cities)
        if (c->resources[CULTURE] != 0)
        {
            sprintf(buf, "%s still holds %d CULTURE after spreading it.", c->name, c->resources[CULTURE]);
            message = std::string(buf);
            return 0;
        }

    // Rings, Q 4: 4/4 at ring 1, 4/9 at ring 2, 4/16 = threshold at ring 3 (not above it).
    // A diagonal ring is the same distance as a side one.
    if (!expect(0,-19, 0, "ring 1 side (Q 4: 1.0)")) return 0;
    if (!expect(0,-18, 0, "ring 2 side (Q 4: 0.44)")) return 0;
    if (!expect(2,-18, 0, "ring 2 diagonal (Q 4: 0.44, same as the side)")) return 0;
    if (!expect(-2,-22, 0, "ring 2 other diagonal")) return 0;
    if (!expect(0,-17, FREE_LAND, "ring 3 (Q 4: 0.25 is not above the threshold)")) return 0;
    if (!expect(3,-23, FREE_LAND, "ring 3 diagonal")) return 0;

    // Terrain: Q 9 reaches path length 5. On grass, 4 tiles west is owned (9/25); 4 tiles east
    // lies behind the ridge (entering it costs 3: path 6) or the strait (no land path in reach).
    if (!expect(10,-24, 0, "4 west of Ridge on grass (9/25)")) return 0;
    if (!expect(10,-16, FREE_LAND, "4 east of Ridge, behind the mountains (path 6 > reach 5)")) return 0;
    if (!expect(-10,-24, 0, "4 west of Strait on grass")) return 0;
    if (!expect(-10,-16, FREE_LAND, "4 east of Strait, across the water")) return 0;

    // Adding up: each of PairA/PairB alone gives the tiles between them exactly 0.25; together 0.5.
    if (!expect(0,1, 0, "between PairA and PairB: 0.25 + 0.25")) return 0;
    if (!expect(1,1, 0, "diagonally between PairA and PairB")) return 0;
    if (!expect(0,-1, FREE_LAND, "next to PairA only: 0.25 alone")) return 0;

    // Border: Weak Q 4 at lon 15, Strong Q 16 at lon 21.  (0,17): 4/9 vs 16/25, Strong wins a tile
    // nearer to Weak; (0,16): 4/4 vs 16/36, Weak keeps its home ground.
    if (!expect(0,16, 0, "next to Weak: 1.0 vs 0.44")) return 0;
    if (!expect(0,17, 1, "2 from Weak, 4 from Strong: 0.44 vs 0.64")) return 0;

    // The worked tile (0,17) went to Strong like an invaded tile: Weak no longer works it.
    if (cities[weakid]->workingOn(0, 2))
    {
        message = std::string("Weak still works (0,17) after Strong's culture took it.");
        return 0;
    }

    // Units and cities keep their tiles.
    if (!expect(1,16, 1, "a Roman warrior stands there although the Vikings' culture wins it")) return 0;
    if (!expect(0,21, 1, "Strong's own city tile")) return 0;
    if (!expect(0,15, 0, "Weak's own city tile, inside Strong's reach")) return 0;
    if (!expect(0,24, 0, "Outpost's city tile: Strong's culture wins it (16/16) but a city keeps its tile")) return 0;

    // Next year nobody produces CULTURE: culture moves out of every tile it took (not worked,
    // no unit) -- releaseOwner(), like an army leaving.
    spreadCulture();
    if (!expect(0,-18, FREE_LAND, "Ring's culture is gone")) return 0;
    if (!expect(0,1, FREE_LAND, "the pair's culture is gone")) return 0;
    if (!expect(0,16, FREE_LAND, "Weak's culture is gone")) return 0;
    if (!expect(1,16, 1, "the warrior still holds its tile")) return 0;

    // The real year: endOfYear() spends the CULTURE a city holds on spreading it.  The test map
    // yields no food, so stock some or every pop-1 city starves and is abandoned.
    for (auto& [k, c] : cities) c->resources[FOOD] = 50;
    cities[ringid]->resources[CULTURE] = 4;
    endOfYear();
    if (cities.find(ringid) == cities.end())
    {
        message = std::string("setup: Ring was abandoned by endOfYear().");
        return 0;
    }
    if (cities[ringid]->resources[CULTURE] != 0 || owner(0,-18) != 0)
    {
        sprintf(buf, "endOfYear() did not spread Ring's CULTURE: it holds %d, (0,-18) owner %d.",
                cities[ringid]->resources[CULTURE], owner(0,-18));
        message = std::string(buf);
        return 0;
    }

    haspassed = true;

    return 0;
}
std::string TestCase_079::title()
{
    return std::string("Culture: square-law heat over the unit-movement path takes and releases land like an army.");

}

bool TestCase_079::done()
{
    return isdone;
}
bool TestCase_079::passed()
{
    return haspassed;
}
std::string TestCase_079::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_079();
}
