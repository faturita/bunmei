//  TestCase_037.cpp
//  bunmei
//
//  Created by Claude on 25/08/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../map.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../mapio.h"

#include "testcase_037.h"

// @Task: New teletype command "/savemap mapnamefile" (usercontrols.cpp) saves the current
// map to saves/mapnamefile.map (mapio.cpp's saveMap(), now parameterized on a path instead
// of always writing "saved_map.dat"). The automatic saveMap() call that used to run every
// time initMap() (gamekernel.cpp) generated a fresh world is removed -- saving the map is
// now only ever a deliberate user action (this command, or the pre-existing /save
// savegame flow, which still writes "saved_map.dat" for -loadgame to pick back up).
//
// This test drives the real teletype command handler (usercontrols.cpp handleKeypress)
// exactly as a key-13 (Enter) event would, with controller.str pre-filled as if the user
// had typed it. It marks one tile with a per-run-unique marker value (time(NULL)^getpid(),
// not a fixed constant -- a stale saves/testcase037map.map left over from an earlier PASSING
// run would otherwise let this test pass even if /savemap silently stopped saving), saves
// via the command, mutates that tile, then loads the saved file back (loadMap(), also now
// parameterized) and checks the original marker came back -- proving the command wrote a
// real, correct map file at "saves/<name>.map", not just the literal string.

extern Map map;
extern Controller controller;
extern int year;
extern Tiles tiles;
extern std::unordered_map<int, Improvement*> improvements;
extern float mapzoom;
extern std::vector<Faction*> factions;

#define TEST_MAPSIZE 1
#define TESTMAP_NAME "testcase037map"
#define TESTMAP_PATH "saves/" TESTMAP_NAME ".map"

TestCase_037::TestCase_037()
{

}

TestCase_037::~TestCase_037()
{

}

int TestCase_037::number()
{
    return 37;
}

void TestCase_037::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initImprovements(improvements);

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255;
    faction->green = 0;
    faction->blue = 0;
    faction->autoPlayer = false;

    factions.push_back(faction);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }

    // A per-run-unique marker (never 0, so it's distinguishable from a cleared/reset cell).
    marker = (int)((time(nullptr) ^ getpid()) & 0x7fffffff) | 1;

    map.set(2,2) = mapcell(LAND);
    map.set(2,2).bioma = GRASSLAND;
    map.set(2,2).resource = marker;

    // Testcases that draw the whole map must set fog-of-war visibility explicitly
    // (drawMap() null-derefs otherwise -- see PROJECT.md's testcase_018 note).
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon).setVisible(0);

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
}

int TestCase_037::check(int year)
{
    ticks++;

    if (ticks == 5)
    {
        // Drive the teletype command exactly as a real Enter keypress would.
        controller.teletype = true;
        controller.str = std::string("/savemap ") + TESTMAP_NAME;
        handleKeypress(13, 0, 0);

        std::ifstream probe(TESTMAP_PATH, std::ios::binary);
        if (!probe)
        {
            isdone = true;
            haspassed = false;
            message = std::string("/savemap did not create " TESTMAP_PATH ".");
            return 0;
        }
        probe.close();
    }

    if (ticks == 6)
    {
        // Mutate the marked tile so the load-back below can only succeed if the file
        // actually holds the original data, not just because nothing changed.
        map.set(2,2).resource = 0;

        loadMap(TESTMAP_PATH);

        isdone = true;

        if (map(2,2).resource != marker)
        {
            haspassed = false;
            message = std::string("Tile (2,2) resource was not restored from saves/ map file -- got a wrong or stale value.");
            return 0;
        }

        haspassed = true;
    }

    return 0;
}
std::string TestCase_037::title()
{
    return std::string("/savemap teletype command saves the map to saves/<name>.map, verified by round-tripping a marked tile.");
}

bool TestCase_037::done()
{
    return isdone;
}
bool TestCase_037::passed()
{
    return haspassed;
}
std::string TestCase_037::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_037();
}
