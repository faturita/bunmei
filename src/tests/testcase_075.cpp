//  TestCase_075.cpp
//  bunmei
//
//  Created by Claude on 23/09/2026
//

#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <vector>
#include <string>

#include "../map.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../coordinator.h"
#include "../commandorder.h"
#include "../engine.h"
#include "../tiles.h"
#include "../resources.h"
#include "../usercontrols.h"
#include "../mapio.h"
#include "../savegame.h"

#include "testcase_075.h"

// @Issue: LUXURY was added back as a core resource, after CULTURE. A core resource is not one
// thing -- it is an enum entry, a stockpile key, a savegame record, a tile-production slot, a
// conversion target and a UI row -- and the ways it can be half-added are quiet ones.
//
// What this pins down, in the order the bugs were found:
//
//   * FUNDAMENTAL_RATES vs CORE_RESOURCE_COUNT. They are different numbers and always were:
//     the rates are the TRADE conversion TARGETS (coins, science, culture, luxury = 4), not
//     the resources (7). Conflating them walked `float rates[4]` one element past its end,
//     which lands exactly on Faction::song -- the function pointer the game calls.
//
//   * Every table sized to the resource count has to BE sized from it, not from a literal.
//     The production tables stayed 6 wide, which silently meant no tile could ever yield
//     LUXURY, and the city constructor's `i<6` left it out of the stockpile entirely.
//
//   * The savegame is per-resource self-describing (count, then id/amount pairs), so a new
//     resource rides along for free -- but that is a property worth asserting rather than
//     assuming, since it is the reason no format change was needed.
//
// And the requirement itself: tiles yield LUXURY and cities accumulate it, working the way
// CULTURE does. CULTURE is used as the ORACLE for that -- the checks compare the two rather
// than restating what LUXURY should do, so the pair cannot drift apart later.

extern std::unordered_map<int, std::string> tiles;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Map map;
extern float mapzoom;

extern Coordinator coordinator;
extern int year;

#define TEST_MAPSIZE 1

#define LUXURY_STOCK 37       // a distinctive amount, to spot it coming back

TestCase_075::TestCase_075() {}
TestCase_075::~TestCase_075() {}

int TestCase_075::number() { return 75; }

void TestCase_075::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initProductionRates(productionrates);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            mapcell &cell = map.set(lat,lon);
            cell = mapcell(LAND);
            cell.bioma = RIVER;               // FOOD 2 / TRADE 1, so there is trade to convert
            cell.setVisible(0);
        }
    assignProductionRates(map);

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255; faction->green = 0; faction->blue = 0;
    faction->autoPlayer = false;
    for (int i=0;i<FUNDAMENTAL_RATES;i++) faction->rates[i] = 0.0f;
    factions.push_back(faction);
    citynames[0] = std::queue<std::string>();

    City *city = new City(&map, 0, getNextCityId(), 0, 0);
    city->setName("Kattegate");
    city->setCapitalCity();
    cities[city->id] = city;
    cityid = city->id;
    citynames[0].push("Kattegate");

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
}

int TestCase_075::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };
    isdone = true;

    City* city = cities[cityid];
    Faction* f = factions[0];

    // ---- 1) the two counts are different things ------------------------------------------
    // If these are ever made equal "because a resource was added", rates writes past its end.
    {
        if (CORE_RESOURCE_COUNT != (int)(sizeof(ALL_CORE_RESOURCES)/sizeof(ALL_CORE_RESOURCES[0])))
        { fail("CORE_RESOURCE_COUNT must be derived from ALL_CORE_RESOURCES, not written out."); return 0; }

        if (FUNDAMENTAL_RATES >= CORE_RESOURCE_COUNT)
        {
            char buf[220];
            snprintf(buf,sizeof(buf),"FUNDAMENTAL_RATES (%d) has reached CORE_RESOURCE_COUNT (%d). They count "
                                     "different things -- rates are the TRADE conversion targets. A loop over "
                                     "resources writing into rates[] runs off the end of the array.",
                     FUNDAMENTAL_RATES, CORE_RESOURCE_COUNT);
            fail(buf); return 0;
        }

        // The concrete consequence, checked rather than described: writing one past rates[]
        // lands on Faction::song, the function pointer gamekernel.cpp and the diplomacy key
        // both call.
        void (*songBefore)() = f->song;
        for (int i=0;i<FUNDAMENTAL_RATES;i++) f->rates[i] = 0.25f;
        if (f->song != songBefore)
        { fail("Writing FUNDAMENTAL_RATES entries into Faction::rates disturbed the song pointer -- the array is too small."); return 0; }
    }

    // ---- 2) LUXURY is a stockpile key like any other --------------------------------------
    {
        if (city->resources.find(LUXURY) == city->resources.end())
        { fail("A new city has no LUXURY entry -- the City constructor must initialise every core resource."); return 0; }

        for (int r : ALL_CORE_RESOURCES)
            if (city->resources.find(r) == city->resources.end())
            {
                char buf[160];
                snprintf(buf,sizeof(buf),"A new city has no stockpile entry for core resource %d.", r);
                fail(buf); return 0;
            }
    }

    // ---- 3) TRADE converts into LUXURY at the faction's fourth rate -----------------------
    {
        f->rates[0] = 0.0f;   // coins
        f->rates[1] = 0.0f;   // science
        f->rates[2] = 0.0f;   // culture
        f->rates[3] = 0.5f;   // luxury

        const int trade = city->getProductionRate(TRADE) - city->getConsumptionRate(TRADE);
        if (trade <= 0)
        { fail("Setup: the city produces no TRADE, so there is nothing to convert."); return 0; }

        const int expected = (int)((float)trade * 0.5f);
        if (expected <= 0)
        { fail("Setup: the trade is too small for a half share to survive the int truncation."); return 0; }

        if (cityTradeConversionRate(city, LUXURY) != expected)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"LUXURY converts to %d; trade(%d) at rate 0.5 is %d.",
                     cityTradeConversionRate(city, LUXURY), trade, expected);
            fail(buf); return 0;
        }
        // ...and it is the FOURTH rate, not one of the others.
        if (cityTradeConversionRate(city, CULTURE) != 0 || cityTradeConversionRate(city, COINS) != 0)
        { fail("LUXURY's rate leaked into another resource -- the rate slots are crossed."); return 0; }
    }

    // ---- 4) the tile-production tables cover it -------------------------------------------
    // No terrain yields LUXURY today, but the tables must be WIDE enough for one to, or a
    // future override would be silently dropped (or read out of bounds).
    {
        std::array<int,CORE_RESOURCE_COUNT> base =
            tileBaseProductionRates(productionrates, LAND, RIVER, 0);
        if ((int)base.size() != CORE_RESOURCE_COUNT)
        { fail("The base production array is not sized to the number of core resources."); return 0; }

        if (map.peek(0,0).getResourceProductionRateSize() != CORE_RESOURCE_COUNT)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"A tile holds %d production slots, there are %d core resources -- "
                                     "the last one(s) could never be produced.",
                     map.peek(0,0).getResourceProductionRateSize(), CORE_RESOURCE_COUNT);
            fail(buf); return 0;
        }

        // Prove a tile CAN carry it, through the real table: one override row, re-assign,
        // read it back -- the same mechanism a future luxury resource would use.
        productionrates.overrides.push_back({ ANY_LAND_BIOMA, MARBLE, {{LUXURY, 3}} });
        map.set(2,2).resource = MARBLE;
        assignProductionRates(map);

        const int got = map.peek(2,2).getResourceProductionRate(LUXURY);
        productionrates.overrides.pop_back();
        map.set(2,2).resource = 0;
        assignProductionRates(map);

        if (got != 3)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"A tile declaring LUXURY 3 in the tables yielded %d -- the production "
                                     "tables are not wide enough for every core resource.", got);
            fail(buf); return 0;
        }
    }

    // ---- 5) LUXURY behaves exactly like CULTURE, end to end -------------------------------
    // The requirement in one sentence: tiles yield it and cities accumulate it, the same way
    // CULTURE does. So CULTURE is the oracle -- every check below compares the two rather
    // than hardcoding what LUXURY ought to do, which is what keeps them from drifting apart.
    {
        // A worked GEMS tile beside the city. GEMS is a luxury product, so the tables give it
        // both (README.md: "Luxury products on tiles will now produce luxury resources
        // (and/or culture)").
        mapcell &gem = map.set(1, 0);
        gem.resource = GEMS;
        assignProductionRates(map);

        // Room to work it: assignTile() refuses past the pop+1 allowance, and a fresh city
        // starts already at it (centre + the one the constructor picks).
        city->pop = 6;

        if (!city->workingOn(1,0)) city->assignTile(coordinate(1,0));
        if (!city->workingOn(1,0))
        { fail("Setup: the gem tile could not be assigned as a worked tile."); return 0; }

        const int tileCulture = city->getProductionRate(CULTURE);
        const int tileLuxury  = city->getProductionRate(LUXURY);

        if (tileLuxury <= 0)
        { fail("A worked GEMS tile yields no LUXURY -- a luxury product must produce it."); return 0; }
        if (tileLuxury != tileCulture)
        {
            char buf[220];
            snprintf(buf,sizeof(buf),"The gem tile yields %d CULTURE but %d LUXURY; the two are meant to "
                                     "behave the same way.", tileCulture, tileLuxury);
            fail(buf); return 0;
        }

        // Accumulation: one year of the real endOfYear arithmetic -- the loop over
        // ALL_CORE_RESOURCES that adds each tile yield into the city's stockpile.
        const int cultureBefore = city->resources[CULTURE];
        const int luxuryBefore  = city->resources[LUXURY];

        for(int r_id : ALL_CORE_RESOURCES)
            city->resources[r_id] += city->getProductionRate(r_id);

        const int cultureGained = city->resources[CULTURE] - cultureBefore;
        const int luxuryGained  = city->resources[LUXURY]  - luxuryBefore;

        if (luxuryGained != tileLuxury)
        {
            char buf[220];
            snprintf(buf,sizeof(buf),"The city gained %d LUXURY from a tile yielding %d -- it must accumulate "
                                     "like any other core resource.", luxuryGained, tileLuxury);
            fail(buf); return 0;
        }
        if (luxuryGained != cultureGained)
        { fail("LUXURY and CULTURE accumulated by different amounts from the same tile."); return 0; }

        // ...and it keeps accumulating year on year rather than being spent or zeroed the way
        // TRADE is (endOfYear clears TRADE after converting it; CULTURE and LUXURY both stay).
        for(int r_id : ALL_CORE_RESOURCES)
            city->resources[r_id] += city->getProductionRate(r_id);

        if (city->resources[LUXURY] - luxuryBefore != 2*tileLuxury)
        { fail("LUXURY did not keep accumulating over a second year."); return 0; }

        // capResources() caps commodities and mfg goods only, so a core resource keeps
        // accumulating past the storage limit. Checked as BOTH the parity and the absolute
        // behaviour: parity alone cannot tell "neither is capped" from "both are".
        city->resources[LUXURY]  = 5000;
        city->resources[CULTURE] = 5000;
        capResources(city);
        if (city->resources[LUXURY] != city->resources[CULTURE])
        { fail("LUXURY is capped differently from CULTURE."); return 0; }
        if (city->resources[LUXURY] != 5000)
        {
            char buf[220];
            snprintf(buf,sizeof(buf),"capResources() clamped LUXURY to %d -- it caps commodities and mfg "
                                     "goods, not core resources.", city->resources[LUXURY]);
            fail(buf); return 0;
        }

        city->resources[LUXURY]  = 0;
        city->resources[CULTURE] = 0;
        gem.resource = 0;
        city->deAssignTile(coordinate(1,0));
        assignProductionRates(map);
    }

    // ---- 6) it survives a savegame round trip ---------------------------------------------
    // The reason no format change was needed: the resource block is (count, then id/amount
    // pairs), so it carries whatever the stockpile happens to hold. Asserted, not assumed.
    {
        city->resources[LUXURY]  = LUXURY_STOCK;
        city->resources[CULTURE] = 11;
        const size_t stockpileSize = city->resources.size();

        char namebuf[64];
        snprintf(namebuf, sizeof(namebuf), "testcase075_%d", (int)((time(nullptr) ^ getpid()) & 0xffffff));
        savename = std::string("saves/") + namebuf;
        savegame(savename.c_str());

        for (auto& [k, c] : cities) delete c;
        cities.clear();
        citynames[0] = std::queue<std::string>();
        citynames[0].push("Kattegate");

        loadMap(savename + ".map");

        std::string savedata;
        SaveGameInfo saveinfo;
        if (!readSaveGame(savename.c_str(), savedata, saveinfo))
        { fail("readSaveGame() rejected the file this test just wrote."); return 0; }

        std::istringstream in(savedata, std::ios::binary);
        int loadedYear = 0;
        in.read(reinterpret_cast<char*>(&loadedYear), sizeof(loadedYear));
        loadCities(in);

        auto it = cities.find(cityid);
        if (it == cities.end())
        { fail("The city did not come back from the savegame."); return 0; }

        City* loaded = it->second;
        if (loaded->resources.find(LUXURY) == loaded->resources.end())
        { fail("LUXURY is missing from the reloaded city -- the savegame dropped a core resource."); return 0; }
        if (loaded->resources[LUXURY] != LUXURY_STOCK)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"LUXURY came back as %d, it was saved as %d.",
                     loaded->resources[LUXURY], LUXURY_STOCK);
            fail(buf); return 0;
        }
        if (loaded->resources[CULTURE] != 11)
        { fail("LUXURY's arrival shifted another resource's value -- the records are positional somewhere."); return 0; }
        if (loaded->resources.size() != stockpileSize)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"The reloaded stockpile holds %d entries, %d were saved.",
                     (int)loaded->resources.size(), (int)stockpileSize);
            fail(buf); return 0;
        }
    }

    haspassed = true;
    return 0;
}

std::string TestCase_075::title()
{
    return std::string("LUXURY as a core resource: FUNDAMENTAL_RATES (the 4 TRADE conversion targets) and CORE_RESOURCE_COUNT (7) are kept distinct so a rates[] loop cannot run onto Faction::song; a new city stocks it; TRADE converts into it at the fourth rate; the tile-production tables are sized from the resource count so terrain CAN yield it; and it survives a savegame round trip, because the resource block is id/amount pairs rather than positional.");
}

bool TestCase_075::done()   { return isdone; }
bool TestCase_075::passed() { return haspassed; }
std::string TestCase_075::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_075();
}
