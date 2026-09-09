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

    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_061::title()
{
    return std::string("Map view offset ('f'/'g') must not leak into world logic: irrigation adjacency reads REAL neighbours (map.peek, not the screen-space map.north/south/east/west), and saveMap writes unshifted cells so a scrolled save reloads identical.");
}

bool TestCase_061::done()   { return isdone; }
bool TestCase_061::passed() { return haspassed; }
std::string TestCase_061::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_061();
}
