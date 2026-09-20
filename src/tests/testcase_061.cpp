//  TestCase_061.cpp
//  bunmei
//
//  Created by Claude on 08/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <string>
#include <vector>

#include "../map.h"
#include "../mapio.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"

#include "testcase_061.h"

// @Issue (issue.png): with the map scrolled east ('f'/'g', which move the viewing faction's
// Faction::mapoffset and therefore Map::offsetlon):
//   (1) a Worker standing next to a river/lake was refused irrigation -- "no river, oasis,
//       lake or irrigated tile nearby" -- because engine.cpp:tileHasWaterOasisOrIrrigationNearby
//       looked at its neighbours through map.north/south/east/west, which go via
//       Map::operator() and ADD the offset. Those helpers take SCREEN coordinates (map.cpp's
//       drawing loops); the worker's latitude/longitude are REAL, so the check was reading
//       tiles `offset` columns away. Fixed by using map.peek() on the real neighbours.
//   (2) saving a scrolled game and reloading it put every unit on the wrong terrain, because
//       mapio.cpp:saveMap read cells through map(lat,lon) (offset applied) while writing
//       UNSHIFTED lat/lon labels, and loadMap read those labels back into map.set() (no
//       offset) -- so the whole map came back rotated by -offset under units and cities, which
//       savegame.cpp stores at their true coordinates. Fixed by saving through map.peek().
//
// Both are checked at offset 0 first (so the test would also catch a regression that broke the
// unshifted case) and then with the map scrolled.
//
// @Issue (issue.png, second report): the same message appeared beside an INNER LAKE on a loaded
// game. Different root cause: gamekernel.cpp tagged a landlocked ocean body as LAKE only where
// `bioma == 0`, but every ocean cell is set to OCEANBIOMA by the generator's "single water
// bioma" pass first -- so the guard was never true and NO cell was ever tagged, on a freshly
// generated map as much as a loaded one. The rule now lives in engine.cpp:tagLakeCells() (so it
// is testable at all -- gamekernel.cpp is not linked into the testcase build) and accepts plain
// open water, bioma 0 or OCEANBIOMA, while still leaving a river mouth alone.

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
#define MAP_SHIFT    5           // how far east the player scrolled ('f' five times)

TestCase_061::TestCase_061() {}
TestCase_061::~TestCase_061() {}

int TestCase_061::number() { return 61; }

void TestCase_061::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);

    // Plain land everywhere, with one RIVER tile at (0,1). The tile under test is (0,0):
    // its EAST neighbour is that river, so irrigation must be allowed there and nowhere far
    // from it.
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
            map.set(lat,lon).bioma = GRASSLAND;
            map.set(lat,lon).setVisible(0);
        }

    map.set(0,1).bioma = RIVER;

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255; faction->green = 0; faction->blue = 0;
    faction->autoPlayer = false;
    factions.push_back(faction);

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
}

int TestCase_061::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    // ---- 1) the irrigation adjacency check must not depend on the view offset -------------
    {
        map.setCenter(0,0);

        if (!tileHasWaterOasisOrIrrigationNearby(0,0))
        { fail("Unshifted: (0,0) is next to the river at (0,1) -- irrigation should be allowed."); return 0; }
        if (tileHasWaterOasisOrIrrigationNearby(0,6))
        { fail("Unshifted: (0,6) is nowhere near the river -- irrigation should be refused."); return 0; }

        // Now scroll the map east, exactly what 'f' does (Faction::mapoffset -> Map::offsetlon).
        // The WORLD has not changed at all, so every answer above must still hold.
        map.setCenter(0,MAP_SHIFT);

        if (!tileHasWaterOasisOrIrrigationNearby(0,0))
        { fail("Scrolled: the river is still at (0,1) -- scrolling the view must not refuse irrigation at (0,0)."); return 0; }
        if (tileHasWaterOasisOrIrrigationNearby(0,6))
        { fail("Scrolled: (0,6) is still far from the river -- irrigation must stay refused."); return 0; }

        // And specifically not the tile the offset would have pointed at: with offsetlon=5 the
        // buggy version answered for (0, 0+5), i.e. it "found" the river from a tile 5 columns
        // west of it.
        if (tileHasWaterOasisOrIrrigationNearby(0,-MAP_SHIFT))
        { fail("Scrolled: (0,-5) is not adjacent to the river -- the check is still adding the view offset."); return 0; }

        map.setCenter(0,0);
    }

    // ---- 2) a map saved while scrolled must reload identical ------------------------------
    {
        // Fingerprint the world as it really is, before any of this.
        std::vector<int> biomaBefore;
        for (int lat=map.minlat; lat<map.maxlat; lat++)
            for (int lon=map.minlon; lon<map.maxlon; lon++)
                biomaBefore.push_back(map.set(lat,lon).bioma);

        // Save with the view scrolled east, as the player would after pressing 'f'.
        map.setCenter(0,MAP_SHIFT);
        saveMap("tmp/testcase_061.map");

        // Wipe the world, then reload. loadMap has no idea what the offset was, and must not
        // need to.
        for (int lat=map.minlat; lat<map.maxlat; lat++)
            for (int lon=map.minlon; lon<map.maxlon; lon++)
                map.set(lat,lon) = mapcell(OCEAN);

        map.setCenter(0,0);
        loadMap("tmp/testcase_061.map");

        int idx = 0, mismatches = 0, firstlat = 0, firstlon = 0;
        for (int lat=map.minlat; lat<map.maxlat; lat++)
            for (int lon=map.minlon; lon<map.maxlon; lon++, idx++)
                if (map.set(lat,lon).bioma != biomaBefore[idx])
                {
                    if (mismatches == 0) { firstlat = lat; firstlon = lon; }
                    mismatches++;
                }

        if (mismatches != 0)
        {
            char buf[240];
            snprintf(buf,sizeof(buf),
                     "A map saved while the view was scrolled came back rotated: %d tiles differ, first at (%d,%d).",
                     mismatches, firstlat, firstlon);
            fail(buf); return 0;
        }

        // The river specifically has to be where it was, or a Worker standing next to it after
        // a load would be refused irrigation for the second time.
        if ((map.set(0,1).bioma & 0xf0) != RIVER)
        { fail("The river is not back at (0,1) after save+load."); return 0; }
        if (!tileHasWaterOasisOrIrrigationNearby(0,0))
        { fail("After save+load a Worker at (0,0) should still be able to irrigate next to the river."); return 0; }
    }

    // ---- 3) landlocked lakes actually get tagged, and make their shore irrigable ----------
    {
        map.setCenter(0,0);

        // A 2x2 inner lake at (5..6, 5..6), the shape of the one in issue.png: OCEAN cells
        // carrying OCEANBIOMA, exactly as the generator leaves them and as a savegame stores
        // them. One of the four is a river mouth, which must be left alone.
        std::vector<coordinate> body;
        for (int lat=5; lat<=6; lat++)
            for (int lon=5; lon<=6; lon++)
            {
                map.set(lat,lon) = mapcell(OCEAN);
                map.set(lat,lon).bioma = OCEANBIOMA;
                body.push_back(coordinate(lat,lon));
            }
        map.set(6,6).bioma = RIVER_MOUTH_S;

        // Before tagging, the land beside it is not irrigable: plain ocean is not a water
        // source, which is the state the game shipped in.
        if (tileHasWaterOasisOrIrrigationNearby(5,4))
        { fail("Plain OCEANBIOMA water must not count as an irrigation source on its own."); return 0; }

        int tagged = tagLakeCells(body);

        if (tagged != 3)
        {
            char buf[160];
            snprintf(buf,sizeof(buf),"tagLakeCells tagged %d of 4 cells; expected 3 (the river mouth is left alone).", tagged);
            fail(buf); return 0;
        }
        if (map.set(5,5).bioma != LAKE || map.set(5,6).bioma != LAKE || map.set(6,5).bioma != LAKE)
        { fail("tagLakeCells should turn plain OCEANBIOMA water into LAKE."); return 0; }
        if (map.set(6,6).bioma != RIVER_MOUTH_S)
        { fail("tagLakeCells must not overwrite a river mouth."); return 0; }

        // Which is the whole point: the shore is now irrigable.
        if (!tileHasWaterOasisOrIrrigationNearby(5,4))
        { fail("Land beside a tagged lake should be irrigable."); return 0; }
        if (!tileHasWaterOasisOrIrrigationNearby(4,5))
        { fail("Land north of a tagged lake should be irrigable."); return 0; }
        if (tileHasWaterOasisOrIrrigationNearby(5,0))
        { fail("Land far from the lake should still be refused."); return 0; }

        // Running it twice must not double count -- LAKE is not plain water any more.
        if (tagLakeCells(body) != 0)
        { fail("tagLakeCells should be idempotent: an already tagged lake has nothing left to tag."); return 0; }

        // And it survives the scroll, same as everything else here.
        map.setCenter(0,MAP_SHIFT);
        if (!tileHasWaterOasisOrIrrigationNearby(5,4))
        { fail("Scrolled: the lake shore must still be irrigable."); return 0; }
        map.setCenter(0,0);
    }

    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_061::title()
{
    return std::string("Irrigation water sources: the map view offset ('f'/'g') must not leak into the adjacency check or into saveMap, and a landlocked ocean body must actually be tagged LAKE (tagLakeCells accepts plain OCEANBIOMA water, not just bioma 0) so its shore is irrigable.");
}

bool TestCase_061::done()   { return isdone; }
bool TestCase_061::passed() { return haspassed; }
std::string TestCase_061::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_061();
}
