//  TestCase_041.cpp
//  bunmei
//
//  Created by Claude on 28/08/2026
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

#include "testcase_041.h"

// @Issue: after -loadgame, every city was missing all but one of its working tiles. Root
// cause (savegame.cpp, loadCities()): the map file (saveMap()/loadMap(), mapio.cpp) already
// serializes each tile's c_id_owner/f_id_owner/owners, and initMap() loads that map BEFORE
// loadCities() runs -- so a city's working tiles are already correctly restored by the time
// loadCities() gets to them. loadCities() then called City::assignWorkingTile(coordinate) for
// each saved (lat,lon) offset anyway. That function is a TOGGLE (the same one the city UI's
// tile click uses): since the tile was ALREADY correctly assigned by the map file, the toggle
// RELEASED it instead of leaving it alone. This happened for every saved tile, wiping every
// working tile except the city centre, which reSetCities() (engine.cpp)'s own workaround
// patches back in every tick regardless of what loadCities() does -- hence exactly ONE
// surviving tile per city, no matter its population. Fix: loadCities() still reads the
// (lat,lon) pairs (to stay aligned with the rest of the stream) but no longer calls
// assignWorkingTile() on them.
//
// This test isolates the exact bug without needing the full game loop: build a city with a
// known set of working tiles (pop=3 -> 4 tiles: the centre plus 3 explicitly assigned), save
// it (savegame() writes the paired map too), then reproduce the REAL load order exactly --
// clear the live tile ownership AND the in-memory city first (so nothing survives by
// accident), reload the map (loadMap(), restoring tile ownership from the file, exactly like
// initMap() does before loadWorldModelling() ever runs), then loadCities() from the same
// stream position loadWorldModelling() would use (right after the year field). The loaded
// city must end up with the same 4 working tiles it was saved with.

extern Map map;
extern std::unordered_map<int, City*> cities;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;
extern Tiles tiles;
extern std::unordered_map<int, Improvement*> improvements;
extern float mapzoom;
extern Coordinator coordinator;
extern int year;

#define TEST_MAPSIZE 1

TestCase_041::TestCase_041()
{

}

TestCase_041::~TestCase_041()
{

}

int TestCase_041::number()
{
    return 41;
}

void TestCase_041::init()
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

    for (int lat=-4;lat<=4;lat++)
        for (int lon=-4;lon<=4;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
            map.set(lat,lon).bioma = GRASSLAND;
        }

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon).setVisible(0);

    resources.push_back(new Resource(0,0,"assets/assets/city/food.png","Food"));
    resources.push_back(new Resource(1,0,"assets/assets/city/production.png","Shields"));
    resources.push_back(new Resource(2,0,"assets/assets/city/trade.png","Trade"));
    resources.push_back(new Resource(3,0,"assets/assets/city/gold.png","Coins"));
    resources.push_back(new Resource(4,0,"assets/assets/city/bulb.png","Science"));
    resources.push_back(new Resource(5,0,"assets/assets/city/culture.png","Culture"));

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255;
    faction->green = 0;
    faction->blue = 0;
    faction->autoPlayer = false;

    factions.push_back(faction);

    citynames[0] = std::queue<std::string>();
    citynames[0].push("Kattegate");
    citynames[0].push("Jorvik");

    City *city = new City(&map, 0, getNextCityId(), 0, 0);
    city->setName("Kattegate");
    city->pop = 3;

    // Centre (0,0) is already working from the constructor. Explicitly assign 3 more,
    // deterministic and collision-free, to reach pop+1=4 total.
    city->assignWorkingTile(coordinate(1,0));
    city->assignWorkingTile(coordinate(-1,0));
    city->assignWorkingTile(coordinate(0,1));

    cities[city->id] = city;
    cityId = city->id;
    expectedWorkingTiles = city->pop + 1;

    if (city->numberOfWorkingTiles() != expectedWorkingTiles)
    {
        isdone = true;
        haspassed = false;
        message = std::string("Test setup itself failed to reach the expected working tile count before saving.");
        return;
    }

    char namebuf[64];
    snprintf(namebuf, sizeof(namebuf), "testcase041_%d", (int)((time(nullptr) ^ getpid()) & 0xffffff));
    savename = std::string("saves/") + namebuf;
    savegame(savename.c_str());

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
}

int TestCase_041::check(int year)
{
    ticks++;

    if (isdone)
        return 0;

    if (ticks == 5)
    {
        // Clear the live tile ownership for every working tile AND the in-memory city, so
        // nothing survives by accident -- only loadMap()+loadCities() can restore them.
        map.set(0,0).releaseCityOwnership();
        map.set(1,0).releaseCityOwnership();
        map.set(-1,0).releaseCityOwnership();
        map.set(0,1).releaseCityOwnership();

        delete cities[cityId];
        cities.erase(cityId);

        // Re-queue a name: loadCities() pops one per loaded city, same as the constructor's
        // caller normally would have already consumed one for this city.
        citynames[0] = std::queue<std::string>();
        citynames[0].push("Kattegate");

        // Reproduce the real load order exactly (gamekernel.cpp): initMap() calls loadMap()
        // BEFORE loadWorldModelling() reads the year field and calls loadCities().
        loadMap(savename + ".map");

        std::ifstream in(savename, std::ios::binary);
        if (!in)
        {
            isdone = true;
            haspassed = false;
            message = std::string("Could not reopen the savegame file for loadCities().");
            return 0;
        }

        int loadedYear = 0;
        in.read(reinterpret_cast<char*>(&loadedYear), sizeof(loadedYear));

        loadCities(in);
        in.close();

        isdone = true;

        auto it = cities.find(cityId);
        if (it == cities.end())
        {
            haspassed = false;
            message = std::string("City was not present after loadCities().");
            return 0;
        }

        City *loaded = it->second;
        int actual = loaded->numberOfWorkingTiles();
        if (actual != expectedWorkingTiles)
        {
            haspassed = false;
            char buf[256];
            snprintf(buf, sizeof(buf), "City has %d working tile(s) after load, expected %d.", actual, expectedWorkingTiles);
            message = std::string(buf);
            return 0;
        }

        haspassed = true;
    }

    return 0;
}
std::string TestCase_041::title()
{
    return std::string("Savegame load: a city's working tiles must survive loadCities(), not be reduced to just the centre (issue.png follow-up).");
}

bool TestCase_041::done()
{
    return isdone;
}
bool TestCase_041::passed()
{
    return haspassed;
}
std::string TestCase_041::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_041();
}
