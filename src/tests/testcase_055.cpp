//  TestCase_055.cpp
//  bunmei
//
//  Created by Claude on 06/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <algorithm>

#include "../map.h"
#include "../units/Unit.h"
#include "../units/Ship.h"
#include "../units/Trireme.h"
#include "../units/Warrior.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../ai.h"
#include "../tiles.h"
#include "../diplomacy.h"
#include "../usercontrols.h"

#include "testcase_055.h"

// @Issue: with Trireme/Galleon now sharing the Unit/Transport interface, an armed Ship moving
// onto an UNDEFENDED enemy (not-contacted -> landSeizure) city tile fell through to
// captureCity() and CAPTURED the city -- a ship has no way to hold ground.
//  Fix: engine.cpp:captureCity() bails out immediately for an OCEANTYPE invader. A defended
//  enemy city is still left to attack() (ships may bombard); a LAND unit still captures
//  normally -- both checked here.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Tiles tiles;
extern DiplomacyTable diplomacy;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_055::TestCase_055() {}
TestCase_055::~TestCase_055() {}

int TestCase_055::number()
{
    return 55;
}

void TestCase_055::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);

    // Ocean everywhere except a 2-tile land bridge: (0,-1) for the land unit, (0,0) the
    // enemy city. The Trireme sits on the ocean tile (0,1) right next to the city.
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon) = mapcell(OCEAN);
    map.set(0,-1) = mapcell(LAND);
    map.set(0, 0) = mapcell(LAND);
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon).setVisible(0);

    Faction *f0 = new Faction();
    f0->id = 0; strcpy(f0->name,"Vikings");
    f0->red = 255; f0->green = 0; f0->blue = 0;
    f0->autoPlayer = false;
    factions.push_back(f0);

    Faction *f1 = new Faction();
    f1->id = 1; strcpy(f1->name,"Romans");
    f1->red = 0; f1->green = 0; f1->blue = 255;
    f1->autoPlayer = false;
    factions.push_back(f1);

    initDiplomacy(diplomacy, 2);           // NO_CONTACT -> landSeizure true (hostile)

    citynames[0] = std::queue<std::string>();
    citynames[1] = std::queue<std::string>();

    // Undefended enemy city on the land bridge.
    City *enemy = new City(&map, 1, getNextCityId(), 0, 0);
    enemy->setName("Roma");
    cities[enemy->id] = enemy;
    enemycityid = enemy->id;

    Trireme *t = new Trireme();
    t->id = getNextUnitId();
    t->faction = 0;
    t->latitude = 0; t->longitude = 1;
    t->availablemoves = t->getUnitMoves();
    units[t->id] = t;
    triremeid = t->id;

    Warrior *w = new Warrior();
    w->id = getNextUnitId();
    w->faction = 0;
    w->latitude = 0; w->longitude = -1;
    w->availablemoves = w->getUnitMoves();
    units[w->id] = w;
    warriorid = w->id;

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = triremeid;
}

int TestCase_055::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    Unit* trireme = units[triremeid];
    Unit* warrior = units[warriorid];
    City* enemy = cities[enemycityid];

    // reSetCities() re-stamps the city centre every tick; make sure it is the enemy's now.
    if (enemy->faction != 1)
    {
        fail("Setup: the enemy city is not owned by faction 1 at check time.");
        return 0;
    }

    // --- (1) a Ship must NOT capture the undefended enemy city -----------------------------
    coordinator.a_u_id = triremeid;
    trireme->availablemoves = trireme->getUnitMoves();
    moveUnit(trireme, enemy->latitude, enemy->longitude);   // step onto (0,0)

    if (enemy->faction != 1)
    {
        fail("A Trireme captured the enemy city -- a naval unit must not be able to.");
        return 0;
    }
    if (trireme->latitude != 0 || trireme->longitude != 1)
    {
        fail("The Trireme moved off the ocean onto the enemy city land tile.");
        return 0;
    }

    // --- (2) a LAND unit still captures the same undefended enemy city --------------------
    coordinator.a_u_id = warriorid;
    warrior->availablemoves = warrior->getUnitMoves();
    moveUnit(warrior, enemy->latitude, enemy->longitude);   // step onto (0,0)

    if (enemy->faction != 0)
    {
        fail("A Warrior could not capture the undefended enemy city -- captureCity() should still work for land units.");
        return 0;
    }

    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_055::title()
{
    return std::string("A Ship (Trireme) cannot capture an enemy city -- captureCity() bails for OCEANTYPE invaders -- while a land Warrior still captures the same undefended city.");
}

bool TestCase_055::done()   { return isdone; }
bool TestCase_055::passed() { return haspassed; }
std::string TestCase_055::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_055();
}
