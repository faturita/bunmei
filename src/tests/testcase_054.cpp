//  TestCase_054.cpp
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
#include <set>

#include "../map.h"
#include "../units/Unit.h"
#include "../units/Settler.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../automation.h"
#include "../tiles.h"
#include "../diplomacy.h"
#include "../usercontrols.h"

#include "testcase_054.h"

// @Issue: an AI Settler that cannot move (blocked by an enemy city/unit -- Settlers never
// attack) kept re-issuing the same impossible step every tick and the game looked frozen.
//  (1) engine.cpp:moveUnit() -- when EVERY move handler fails for an autoPlayer unit, its
//      GoTo is cleared and availablemoves -> 0, so switchUnitIfNoMovesLeft() advances the
//      cursor and the AI re-plans next turn instead of spinning.
//  (2) ai.cpp:findCitySpot() now takes a `faction` and its BFS does NOT expand through
//      tiles owned by ANOTHER faction's city -- so it only returns spots a settler can
//      actually walk to, not ones stranded behind an enemy border.

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

TestCase_054::TestCase_054() {}
TestCase_054::~TestCase_054() {}

int TestCase_054::number()
{
    return 54;
}

void TestCase_054::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);

    // Mostly ocean; a 3-tile-tall LAND strip along the equator. It starts exactly at the
    // faction-0 city (lon -5), so the whole western span is inside that city's CITY_SPACING
    // -- there is NO good city spot west of the faction-1 wall.
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon) = mapcell(OCEAN);
    for(int lat=-1;lat<=1;lat++)
        for (int lon=-5;lon<=12;lon++)
            map.set(lat,lon) = mapcell(LAND);
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon).setVisible(0);

    Faction *f0 = new Faction();
    f0->id = 0; strcpy(f0->name,"Vikings");
    f0->red = 255; f0->green = 0; f0->blue = 0;
    // Left non-auto so the game loop's autoPlayerMoveUnits() doesn't disband our settler
    // before check() runs; check() flips it to true only around the move it is testing.
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

    // Faction 0's own city near the west end, so the near strip is all inside its
    // CITY_SPACING (no good city spot there).
    City *own = new City(&map, 0, getNextCityId(), 0, -5);
    own->setName("Kaupang");
    cities[own->id] = own;

    // Faction 1's city + a foreign-owned wall (lat -1..1, lon -2..0) straddling the strip:
    // an AI settler of faction 0 cannot cross it, so any good spot to the east is stranded.
    City *enemy = new City(&map, 1, getNextCityId(), 0, -1);
    enemy->setName("Roma");
    cities[enemy->id] = enemy;
    enemycityid = enemy->id;
    for(int lat=-1;lat<=1;lat++)
        for(int lon=-2;lon<=0;lon++)
            map.set(lat,lon).setCityOwnership(1, enemy->id);

    Settler *s = new Settler();
    s->id = getNextUnitId();
    s->faction = 0;
    s->latitude = 0; s->longitude = -4;
    s->availablemoves = s->getUnitMoves();
    units[s->id] = s;
    settlerid = s->id;

    mapzoom = 2;
    centermapinmap(0,0);
    // a_f_id = 1 (a non-autoPlayer faction) keeps the game loop's AI move paths off our
    // settler -- we drive moveUnit() directly. a_u_id points at the settler so land()'s
    // `units[coordinator.a_u_id]` lookup inside moveUnit() resolves (it would otherwise
    // insert a null into the units map).
    coordinator.a_f_id = 1;
    coordinator.a_u_id = s->id;
}

int TestCase_054::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    Unit* settlerU = units[settlerid];
    Settler* settler = dynamic_cast<Settler*>(settlerU);
    City* enemy = cities[enemycityid];

    // The game loop runs every tick and its getRandomInteger() is seeded from
    // std::random_device (yamathutil.cpp) -- so reSetCities()'s working-tile churn is
    // non-deterministic and can momentarily release the foreign wall's ownership. Re-stamp
    // it here so the tile-ownership precondition of the move under test always holds.
    auto stampWall = [&]{
        for(int lat=-1;lat<=1;lat++)
            for(int lon=-2;lon<=0;lon++)
                map.set(lat,lon).setCityOwnership(1, enemy->id);
    };
    stampWall();

    // --- (2) findCitySpot: faction-aware, does not cross enemy borders ---------------------
    {
        bool foundForUs = true;
        findCitySpot(coordinate(0,-4), 0, foundForUs);
        if (foundForUs)
        {
            fail("findCitySpot(faction 0) found a spot -- it should be blocked: the only good spots are east of the faction-1 wall.");
            return 0;
        }

        // Same wall, but for faction 1 it is its OWN territory -> BFS crosses it and reaches
        // the eastern good spots.
        bool foundForThem = false;
        coordinate spot = findCitySpot(coordinate(0,-4), 1, foundForThem);
        if (!foundForThem || spot.lon <= 0)
        {
            char buf[160];
            snprintf(buf,sizeof(buf),"findCitySpot(faction 1) should walk through its own wall to an eastern spot (found=%d, lon=%d).",
                     foundForThem, spot.lon);
            fail(buf);
            return 0;
        }
    }

    // --- (1) moveUnit: a blocked AI settler drops its GoTo + moves instead of spinning ----
    factions[0]->autoPlayer = true;
    settler->availablemoves = settler->getUnitMoves();
    settler->goTo(0, 8);                       // target east, past the enemy city
    if (!settler->isAuto())
    {
        factions[0]->autoPlayer = false;
        fail("Setup: settler->goTo() did not put the settler in auto mode.");
        return 0;
    }

    // Try to step onto the (undefended) enemy city tile -- a Settler has 0 attack, so every
    // handler in moveUnit() fails.
    stampWall();
    moveUnit(settler, enemy->latitude, enemy->longitude);
    factions[0]->autoPlayer = false;

    if (settler->latitude != 0 || settler->longitude != -4)
    {
        fail("The Settler moved onto/through the enemy city tile -- it should not have.");
        return 0;
    }
    if (settler->availablemoves != 0)
    {
        char buf[128];
        snprintf(buf,sizeof(buf),"After the blocked move the AI Settler still has %.1f moves -- expected 0 (would keep re-issuing the same step).",
                 settler->availablemoves);
        fail(buf);
        return 0;
    }
    if (settler->isAuto())
    {
        fail("After the blocked move the AI Settler is still in auto mode -- its GoTo should have been reset.");
        return 0;
    }

    // A NON-automated unit in the same situation must be left alone (it's the human's call).
    settler->availablemoves = settler->getUnitMoves();
    settler->goTo(0, 8);
    stampWall();
    moveUnit(settler, enemy->latitude, enemy->longitude);
    if (settler->availablemoves == 0 || !settler->isAuto())
    {
        fail("A blocked move zeroed moves / reset GoTo for a NON-autoPlayer unit -- that should only happen for the AI.");
        return 0;
    }

    settler->resetGoTo();   // leave it clean for the remaining game-loop ticks
    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_054::title()
{
    return std::string("A blocked AI Settler no longer freezes the game: moveUnit() zeroes its moves + clears its GoTo; findCitySpot() is faction-aware and won't route through an enemy city's territory.");
}

bool TestCase_054::done()   { return isdone; }
bool TestCase_054::passed() { return haspassed; }
std::string TestCase_054::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_054();
}
