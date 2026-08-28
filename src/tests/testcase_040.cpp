//  TestCase_040.cpp
//  bunmei
//
//  Created by Claude on 27/08/2026
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
#include "../savegame.h"

#include "testcase_040.h"

// @Task: savegames.
//   1. Savegame files must be stored in saves/.
//   2. The '/save [filename]' teletype command must effectively store the file there.
//   3. The map must be stored in saves/ too.
//   4. The map save must also carry per-faction visibility (fog of war) and improvements
//      (roads/mines/irrigation/etc), which saveMap()/loadMap() never persisted before.
//
// savegame() (savegame.cpp) now prepends "saves/" to whatever filename it's given (unless
// already prefixed) and creates the directory if needed -- same pattern saveMap() (mapio.cpp,
// task #28) already uses -- then writes the paired map to "<path>.map" via saveMap(), instead
// of the old shared, always-overwritten "saved_map.dat". usercontrols.cpp's '/save' handler
// was also fixed along the way: its old "no filename given" detection
// (`controller.str.length()<=4`) could never be true ("/save" alone is already 5 chars), so
// typing bare "/save" silently saved to an EMPTY filename; now parsed with istringstream, same
// pattern '/savemap' (task #28) already uses. mapio.cpp's saveMap()/loadMap() gained two new
// per-cell fields: `improvements` (a plain int, memcpy'd like the others) and `visible`
// (vector<bool>, bit-packed so NOT memcpy-safe -- written/read one byte per entry, size-prefixed
// so a per-cell "how many factions have explored this" count survives too).
//
// This test drives the real '/save' command through handleKeypress() (Enter-key event, not a
// synthetic call), with a per-run-unique filename (time(NULL)^getpid(), same reasoning as
// testcase_037's marker: a fixed name would let a stale saves/ file from an earlier PASSING
// run mask a future regression). Verifies: (1) the savegame file lands under saves/, NOT at
// the bare name in the working directory; (2) a paired saves/<name>.map exists; (3)/(4) that
// map file, loaded back after the live map is scrambled, restores a tile's exact improvements
// bitmap and per-faction visibility (including a SKIPPED faction in between two explored ones,
// exercising the sparse vector<bool> resize/serialization path).

extern Map map;
extern Controller controller;
extern int year;
extern Tiles tiles;
extern std::unordered_map<int, Improvement*> improvements;
extern float mapzoom;
extern std::vector<Faction*> factions;
extern Coordinator coordinator;

#define TEST_MAPSIZE 1

TestCase_040::TestCase_040()
{

}

TestCase_040::~TestCase_040()
{

}

int TestCase_040::number()
{
    return 40;
}

void TestCase_040::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initImprovements(improvements);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }

    // The marked tile: a distinctive improvements bitmap (Irrigation+Road) and per-faction
    // visibility with a gap (0 and 2 explored it, 1 never did) -- both must round-trip.
    map.set(2,2) = mapcell(LAND);
    map.set(2,2).bioma = GRASSLAND;
    map.set(2,2).resource = SILK;
    map.set(2,2).improvements = IRRIGATION | ROAD;
    map.set(2,2).setVisible(0);
    map.set(2,2).setVisible(2);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon).setVisible(0);

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255;
    faction->green = 0;
    faction->blue = 0;
    faction->autoPlayer = false;

    factions.push_back(faction);

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
}

int TestCase_040::check(int year)
{
    ticks++;

    if (ticks == 5)
    {
        char namebuf[64];
        snprintf(namebuf, sizeof(namebuf), "testcase040_%d", (int)((time(nullptr) ^ getpid()) & 0xffffff));
        savename = namebuf;

        controller.teletype = true;
        controller.str = std::string("/save ") + savename;
        handleKeypress(13, 0, 0);

        // (1)/(2): the file must land under saves/, not at the bare name in the working
        // directory (the pre-fix behaviour: savegame() wrote wherever it was told, verbatim).
        std::ifstream stray(savename, std::ios::binary);
        if (stray)
        {
            isdone = true;
            haspassed = false;
            message = std::string("/save wrote a stray file outside saves/: ") + savename;
            return 0;
        }

        std::ifstream probe("saves/" + savename, std::ios::binary);
        if (!probe)
        {
            isdone = true;
            haspassed = false;
            message = std::string("/save did not create saves/") + savename;
            return 0;
        }
        probe.close();

        // (3): the paired map file must exist under saves/ too.
        std::ifstream mapprobe("saves/" + savename + ".map", std::ios::binary);
        if (!mapprobe)
        {
            isdone = true;
            haspassed = false;
            message = std::string("/save did not create the paired saves/") + savename + ".map";
            return 0;
        }
        mapprobe.close();
    }

    if (ticks == 6)
    {
        isdone = true;

        // Scramble the live tile so the load-back below can only succeed if the file
        // actually holds the original data.
        map.set(2,2) = mapcell(LAND);
        map.set(2,2).bioma = HILLS;

        loadMap("saves/" + savename + ".map");

        // (4): improvements bitmap must be restored exactly.
        if (map(2,2).improvements != (IRRIGATION | ROAD))
        {
            haspassed = false;
            message = std::string("Tile (2,2) improvements were not restored from the saved map.");
            return 0;
        }

        // (4): per-faction visibility must be restored, including the gap (faction 1).
        if (!map(2,2).isVisible(0) || map(2,2).isVisible(1) || !map(2,2).isVisible(2))
        {
            haspassed = false;
            message = std::string("Tile (2,2) per-faction visibility was not restored correctly from the saved map.");
            return 0;
        }

        haspassed = true;
    }

    return 0;
}
std::string TestCase_040::title()
{
    return std::string("Savegames: /save routes into saves/, saves a paired map, and the map preserves improvements + per-faction visibility.");
}

bool TestCase_040::done()
{
    return isdone;
}
bool TestCase_040::passed()
{
    return haspassed;
}
std::string TestCase_040::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_040();
}
