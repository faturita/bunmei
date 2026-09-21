//  TestCase_064.cpp
//  bunmei
//
//  Created by Claude on 20/09/2026
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <vector>
#include <queue>

#include "../map.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../improvements.h"
#include "../usercontrols.h"
#include "../mapio.h"
#include "../savegame.h"

#include "testcase_064.h"

// @Issue: after -loadgame (reported on saves/game), EVERY city came back flagged as its
// faction's capital. The savegame file itself was fine -- saves/game stores exactly one
// capital per faction (Kattegate for the Vikings, Roma for the Romans) -- so this was purely
// a load bug: savegame.cpp's loadCities() read the bool into a local and then called
// c->setCapitalCity() UNCONDITIONALLY, discarding what it had just read.
//
// Not cosmetic. endOfYear() (bunmei.cpp) charges the faction's whole salary bill to each
// capital city, so a four-city faction paid its salaries four times over every year after a
// load, and factionTreasury() (engine.cpp) returned whichever "capital" the unordered_map
// happened to yield first instead of the real one.
//
// Driven through the REAL format -- savegame() to a file, then the same load order
// gamekernel.cpp uses (loadMap(), read the year, loadCities()) -- rather than a synthetic
// City, so the bug could not hide in the round trip. Two factions with two cities each, one
// capital apiece, which is the shape that distinguishes "the flag survived" from "everything
// is a capital" AND from "nothing is".

extern std::unordered_map<int, std::string> tiles;
extern std::unordered_map<int, Improvement*> improvements;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Map map;
extern float mapzoom;

extern Coordinator coordinator;
extern int year;

#define TEST_MAPSIZE 1

// (name, faction, lat, lon, capital) -- the state that has to survive the round trip.
struct ExpectedCity { const char* name; int faction; int lat; int lon; bool capital; };
static const ExpectedCity EXPECTED[] = {
    { "Kattegate", 0,  0,  0, true  },
    { "Jorvik",    0,  3,  2, false },
    { "Roma",      1, -3, -2, true  },
    { "Caesarea",  1, -3,  1, false }
};
#define EXPECTED_COUNT ((int)(sizeof(EXPECTED)/sizeof(EXPECTED[0])))

TestCase_064::TestCase_064() {}
TestCase_064::~TestCase_064() {}

int TestCase_064::number() { return 64; }

void TestCase_064::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initImprovements(improvements);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon) = mapcell(OCEAN);

    for (int lat=-6;lat<=6;lat++)
        for (int lon=-6;lon<=6;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
            map.set(lat,lon).bioma = GRASSLAND;
        }

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon).setVisible(0);

    for (int f=0;f<2;f++)
    {
        Faction *faction = new Faction();
        faction->id = f;
        strcpy(faction->name, f == 0 ? "Vikings" : "Romans");
        faction->red = 255; faction->green = 0; faction->blue = 0;
        faction->autoPlayer = false;
        factions.push_back(faction);

        citynames[f] = std::queue<std::string>();
    }

    for (int i=0;i<EXPECTED_COUNT;i++)
    {
        City *city = new City(&map, EXPECTED[i].faction, getNextCityId(),
                              EXPECTED[i].lat, EXPECTED[i].lon);
        city->setName(EXPECTED[i].name);
        city->pop = 2;
        if (EXPECTED[i].capital)
            city->setCapitalCity();
        cities[city->id] = city;

        // loadCities() pops one queued name per city it reads, same as the founding path
        // would already have consumed one for each of these.
        citynames[EXPECTED[i].faction].push(EXPECTED[i].name);
    }

    char namebuf[64];
    snprintf(namebuf, sizeof(namebuf), "testcase064_%d", (int)((time(nullptr) ^ getpid()) & 0xffffff));
    savename = std::string("saves/") + namebuf;
    savegame(savename.c_str());

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
}

int TestCase_064::check(int year)
{
    ticks++;

    if (isdone) return 0;
    if (ticks != 5) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    isdone = true;

    // ---- wipe: nothing may survive in memory, only the file can restore it --------------
    for (auto& [k, c] : cities)
        delete c;
    cities.clear();

    for (int f=0;f<2;f++)
        citynames[f] = std::queue<std::string>();
    for (int i=0;i<EXPECTED_COUNT;i++)
        citynames[EXPECTED[i].faction].push(EXPECTED[i].name);

    // ---- read back, in the order gamekernel.cpp uses -------------------------------------
    loadMap(savename + ".map");

    // Through readSaveGame(), like the game does: the file starts with a general header
    // (magic + payload length + MD5) and the payload with its own version header, so the
    // data is read out of the verified payload instead of straight off the stream.
    std::string savedata;
    SaveGameInfo saveinfo;
    if (!readSaveGame(savename.c_str(), savedata, saveinfo))
    { fail("readSaveGame() rejected the file this test just wrote."); return 0; }
    std::istringstream in(savedata, std::ios::binary);

    int loadedYear = 0;
    in.read(reinterpret_cast<char*>(&loadedYear), sizeof(loadedYear));
    loadCities(in);

    if ((int)cities.size() != EXPECTED_COUNT)
    {
        char buf[200];
        snprintf(buf,sizeof(buf),"%d cities came back from the savegame, saved %d.",
                 (int)cities.size(), EXPECTED_COUNT);
        fail(buf); return 0;
    }

    // ---- every city's capital flag must be exactly what was saved ------------------------
    for (int i=0;i<EXPECTED_COUNT;i++)
    {
        City* loaded = nullptr;
        for (auto& [k, c] : cities)
            if (strcmp(c->name, EXPECTED[i].name) == 0)
                loaded = c;

        if (loaded == nullptr)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"City '%s' was not present after loadCities().", EXPECTED[i].name);
            fail(buf); return 0;
        }
        if (loaded->isCapitalCity() != EXPECTED[i].capital)
        {
            char buf[240];
            snprintf(buf,sizeof(buf),
                     "'%s' came back %sa capital city; it was saved %sa capital.",
                     EXPECTED[i].name,
                     loaded->isCapitalCity() ? "" : "NOT ",
                     EXPECTED[i].capital ? "as " : "NOT as ");
            fail(buf); return 0;
        }
    }

    // ---- and each faction must end with exactly ONE ---------------------------------------
    // Stated separately because it is the invariant the game's consumers actually rely on:
    // endOfYear() bills the salaries to every capital it finds, and factionTreasury() returns
    // the first one. Two capitals in a faction is a double charge; none is a missed one.
    for (int f=0;f<2;f++)
    {
        int capitals = 0;
        for (auto& [k, c] : cities)
            if (c->faction == f && c->isCapitalCity())
                capitals++;

        if (capitals != 1)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"Faction %d has %d capital cities after the load, expected exactly 1.",
                     f, capitals);
            fail(buf); return 0;
        }
    }

    haspassed = true;
    return 0;
}

std::string TestCase_064::title()
{
    return std::string("Savegame load: a city's CAPITAL flag must survive loadCities() -- loadCities() read the saved bool and then called setCapitalCity() unconditionally, so every city came back a capital and every city paid the faction's full salary bill each year.");
}

bool TestCase_064::done()   { return isdone; }
bool TestCase_064::passed() { return haspassed; }
std::string TestCase_064::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_064();
}
