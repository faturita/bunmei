//  TestCase_052.cpp
//  bunmei
//
//  Created by Claude on 05/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <string>
#include <set>

#include "../map.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../tiles.h"
#include "../ai.h"
#include "../usercontrols.h"

#include "testcase_052.h"

// @Task: More civs (18 total). Two testable pieces of that change:
//   1. tiles.cpp:initNaming() now fills a city-name queue for every one of the 18
//      civilizations (was 8) -- no faction id is left without names.
//   2. ai.cpp:pickFactionStartTiles(n) -- new shared helper used by both initUnits() copies
//      (gamekernel.cpp / simulate.cpp) so two civilizations never start on the same tile:
//      it returns n DISTINCT random LAND tiles (fewer only if the map has fewer land tiles).
//   (FACTION_DEFINITIONS / -civs live in gamekernel.cpp, not the testcase build -- verified
//    with a live `./bunmei -civs 18` run instead.)

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

TestCase_052::TestCase_052() {}
TestCase_052::~TestCase_052() {}

int TestCase_052::number()
{
    return 52;
}

void TestCase_052::init()
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

    // The game loop indexes factions[coordinator.a_f_id] every tick -- one is enough here.
    Faction *f = new Faction();
    f->id = 0; strcpy(f->name,"Vikings");
    f->red = 255; f->green = 0; f->blue = 0;
    f->autoPlayer = false;
    factions.push_back(f);

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.v_f_id = 0;
}

int TestCase_052::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    // ---- 1) initNaming(): a non-empty, equal-length name queue for all 18 civs ----
    citynames.clear();
    initNaming(citynames);

    if (citynames.size() != 18)
    {
        char buf[128];
        snprintf(buf,sizeof(buf),"initNaming filled %zu city-name queues, expected 18.", citynames.size());
        fail(buf);
        return 0;
    }

    size_t expected = citynames[0].size();
    if (expected == 0)
    {
        fail("initNaming left civ 0's city-name queue empty.");
        return 0;
    }
    for (int f = 0; f < 18; f++)
    {
        if (citynames.find(f) == citynames.end() || citynames[f].size() != expected)
        {
            char buf[128];
            snprintf(buf,sizeof(buf),"civ %d has %zu city names, expected %zu (same as civ 0).",
                     f, citynames.count(f) ? citynames[f].size() : 0, expected);
            fail(buf);
            return 0;
        }
    }
    // Spot-check a couple of the new civs' first names.
    if (citynames[8].front() != "Moscow" || citynames[16].front() != "Kyoto" || citynames[17].front() != "Madrid")
    {
        fail("A new civ's first city name is not what initNaming should have queued (Russians/Japanese/Spanish).");
        return 0;
    }

    // ---- 2) pickFactionStartTiles(): distinct LAND tiles ----
    // Carve a known handful of LAND tiles into an otherwise all-ocean map.
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon) = mapcell(OCEAN);

    std::set<std::pair<int,int>> landset;
    for (int lat = -3; lat <= 3; lat++)
        for (int lon = -3; lon <= 3; lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
            landset.insert({lat,lon});
        }
    const int LANDCOUNT = (int)landset.size();   // 49

    std::vector<coordinate> starts = pickFactionStartTiles(18);
    if ((int)starts.size() != 18)
    {
        char buf[128];
        snprintf(buf,sizeof(buf),"pickFactionStartTiles(18) returned %zu tiles, expected 18.", starts.size());
        fail(buf);
        return 0;
    }
    std::set<std::pair<int,int>> seen;
    for (auto& c : starts)
    {
        if (landset.find({c.lat,c.lon}) == landset.end())
        {
            fail("pickFactionStartTiles returned a non-LAND tile.");
            return 0;
        }
        if (!seen.insert({c.lat,c.lon}).second)
        {
            fail("pickFactionStartTiles returned the same tile twice.");
            return 0;
        }
    }

    // Asking for more starts than there are land tiles caps at the land count, still distinct.
    std::vector<coordinate> capped = pickFactionStartTiles(LANDCOUNT + 5);
    seen.clear();
    for (auto& c : capped)
        if (!seen.insert({c.lat,c.lon}).second)
        {
            fail("pickFactionStartTiles(> land count) still returned a duplicate tile.");
            return 0;
        }
    if ((int)capped.size() != LANDCOUNT)
    {
        char buf[128];
        snprintf(buf,sizeof(buf),"pickFactionStartTiles(%d) returned %zu, expected the land count %d.",
                 LANDCOUNT + 5, capped.size(), LANDCOUNT);
        fail(buf);
        return 0;
    }

    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_052::title()
{
    return std::string("More civs: initNaming() names all 18 civilizations; pickFactionStartTiles() returns that many DISTINCT land tiles so no two civs share a start.");
}

bool TestCase_052::done()   { return isdone; }
bool TestCase_052::passed() { return haspassed; }
std::string TestCase_052::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_052();
}
