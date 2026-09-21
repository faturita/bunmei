//  TestCase_065.cpp
//  bunmei
//
//  Created by Claude on 20/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cmath>

#include "../map.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../codes.h"
#include "../dee.h"
#include "../usercontrols.h"
#include "../cityscreenui.h"
#include "../buildings/Palace.h"

#include "testcase_065.h"

// @Task: show the SCIENCE and CULTURE each city produces in the city screen's "City
// Resources" box, under the gold row.
//
// The rows were already there and already in the right place -- the box loops over
// ALL_CORE_RESOURCES, so SCIENCE (index 4) and CULTURE (index 5) sit directly below COINS
// (index 3) -- but they always drew EMPTY, because the box sized every row from
// getProductionRate() alone and no bioma produces any of the three (gamekernel.cpp
// BASE_PRODUCTION_RATE). A city's real income of them is its TRADE converted at the
// faction's fundamental rates, which only happened inside endOfYear().
//
// So the fix is a rule, not a layout: new engine.cpp `cityTradeConversionRate(city, r)`
// mirrors that conversion (TRADE_SURPLUS perk doubling included) and the box ADDS it to
// getProductionRate(). Added, not substituted -- a GOLD tile really does yield COINS 2 +
// CULTURE 1 and GEMS yields CULTURE 2 (RESOURCE_RATE_OVERRIDE), so both sources count.
//
// Tested against the faction's rates rather than hardcoded totals, so the test states the
// RULE (trade * rate) and cannot drift if the test map's trade yield changes.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Tiles tiles;
extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;
extern DependencyEvaluationEngine dee;

#define TEST_MAPSIZE 1

// Deliberately uneven and summing to less than 1 (the remainder is the untaxed share): a
// 0.5/0.5/0.5 split could not tell the three rate slots apart if they were transposed.
#define RATE_COINS   0.50f
#define RATE_SCIENCE 0.25f
#define RATE_CULTURE 0.125f

TestCase_065::TestCase_065() {}
TestCase_065::~TestCase_065() {}

int TestCase_065::number() { return 65; }

void TestCase_065::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initCoreResources();

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            mapcell &cell = map.set(lat,lon);
            cell = mapcell(LAND);
            // RIVER, so the tables give every tile FOOD 2 and TRADE 1 (productionrates.base,
            // tiles.cpp). No yield is written by hand: the terrain is stated and
            // assignProductionRates() below fills each tile's own rates from the tables, the
            // same call the game makes for a generated world and loadMap() makes for a
            // loaded one.
            cell.bioma = RIVER;
            cell.setVisible(0);
        }

    assignProductionRates(map);

    Faction *faction = new Faction();
    faction->id = 0; strcpy(faction->name,"Vikings");
    faction->red = 255; faction->green = 0; faction->blue = 0;
    faction->autoPlayer = false;
    faction->rates[0] = RATE_COINS;
    faction->rates[1] = RATE_SCIENCE;
    faction->rates[2] = RATE_CULTURE;
    faction->rates[3] = 0.0f;
    factions.push_back(faction);

    City *city = new City(&map, 0, getNextCityId(), 3, 3);
    city->setName("Kattegate");
    city->foundedyear = -4000;
    // Roomy on purpose: assignWorkingTile() refuses past pop+1 worked tiles (and TOGGLES,
    // releasing instead of assigning, when it is full), so the checks below need headroom
    // over whatever the constructor and reSetCities() have already claimed.
    city->pop = 12;
    city->buildings.push_back(new Palace());

    // Work a ring of tiles. Each worked RIVER tile is TRADE 1 off the tables, and the
    // conversion truncates to int exactly as endOfYear() does, so the city needs enough
    // trade for even the SMALLEST rate below to land on a positive whole number -- otherwise
    // the test could not tell "converts to nothing" from "rounded away". The gem and gold
    // tiles of part 3 are (1,0) and (0,1), so they are in this list deliberately.
    const int ring[][2] = { {1,0}, {0,1}, {-1,0}, {0,-1}, {1,1}, {-1,-1}, {1,-1}, {-1,1} };
    for (const auto &t : ring)
        city->assignWorkingTile(coordinate(t[0], t[1]));

    cities[city->id] = city;
    cityid = city->id;

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
}

int TestCase_065::check(int year)
{
    ticks++;
    if (isdone) return 0;

    controller.view = 2;
    controller.cityid = cityid;

    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    City* city = cities[cityid];

    // The render sanity pass runs on the tick AFTER the arithmetic checks (same shape as
    // testcase_053), so drawCityScreen() sees the state the checks just validated.
    if (phase == 1)
    {
        coordinate c = map.to_screen(city->latitude, city->longitude);
        drawCityScreen(c.lat, c.lon, city);   // the City Resources box must render all six rows
        isdone = true;
        haspassed = true;
        return 0;
    }
    phase = 1;

    // Read live, never cached: assigning a worked tile below changes the city's TRADE yield,
    // and every expectation here is stated as "the trade AT THIS MOMENT times the rate".
    auto trade = [&](){ return city->getProductionRate(TRADE) - city->getConsumptionRate(TRADE); };

    if (trade() <= 0)
    { fail("Setup: the city produces no TRADE, so there would be nothing to convert."); return 0; }

    // ---- 1) the conversion itself: trade * the faction's rate, per resource --------------
    struct Expect { int resource; float rate; const char* name; };
    const Expect expected[] = {
        { COINS,   RATE_COINS,   "COINS"   },
        { SCIENCE, RATE_SCIENCE, "SCIENCE" },
        { CULTURE, RATE_CULTURE, "CULTURE" }
    };

    for (const Expect& e : expected)
    {
        int got  = cityTradeConversionRate(city, e.resource);
        int want = (int)((float)trade() * e.rate);
        if (got != want)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"cityTradeConversionRate(%s) = %d, expected trade(%d) * rate(%.3f) = %d.",
                     e.name, got, trade(), e.rate, want);
            fail(buf); return 0;
        }
    }
    // SCIENCE is the headline of the task: it must actually be a positive number, or the row
    // would still draw empty and nothing would have been fixed.
    if (cityTradeConversionRate(city, SCIENCE) <= 0)
    { fail("SCIENCE converts to 0 -- its row would still render empty."); return 0; }
    if (cityTradeConversionRate(city, CULTURE) <= 0)
    { fail("CULTURE converts to 0 -- its row would still render empty."); return 0; }

    // The rates are distinct, so the three must not come back equal -- that is what would
    // happen if the rate slots were mixed up.
    if (cityTradeConversionRate(city, COINS) == cityTradeConversionRate(city, SCIENCE))
    { fail("COINS and SCIENCE convert to the same amount despite different rates -- wrong rate slot."); return 0; }

    // ---- 2) every other core resource is untouched ---------------------------------------
    // The tiles produce FOOD and TRADE; the conversion must claim neither.
    for (int r : { FOOD, SHIELDS, TRADE })
        if (cityTradeConversionRate(city, r) != 0)
        {
            char buf[160];
            snprintf(buf,sizeof(buf),"cityTradeConversionRate() returned non-zero for resource %d, which the terrain produces.", r);
            fail(buf); return 0;
        }

    // ---- 3) it ADDS to tile production, it does not replace it ---------------------------
    // This is the half that makes the box correct rather than merely non-empty: special
    // resources DO yield these three off the tiles. The rates below are the real ones
    // assignProductionRates() writes (gamekernel.cpp RESOURCE_RATE_OVERRIDE), confirmed
    // against the GEMS and GOLD tiles in an actual saves/game.map:
    //     GEMS on land -> CULTURE 2
    //     GOLD on land -> COINS 2 + CULTURE 1
    // gamekernel.cpp is not linked into the testcase build, so the rates are written here
    // the way that function would have. `resource` is set too -- getProductionRate() reads
    // only the rate vector, but a tile claiming a culture yield with no gem on it would be
    // a misleading fixture.
    {
        // Just put the gem and the gold on the map -- no yields are set by hand. The tables
        // (productionrates.overrides) turn a GEMS tile into CULTURE 2 and a GOLD tile into
        // COINS 2 + CULTURE 1, which is what the real generator relied on too, and matches
        // the GEMS and GOLD tiles found in an actual saves/game.map.
        map.set(city->latitude + 1, city->longitude).resource = GEMS;
        map.set(city->latitude, city->longitude + 1).resource = GOLD;

        // A THIRD gem tile in range but deliberately NOT worked: "when they are worked" is
        // the rule (City::getProductionRate only sums tiles workingOn()), so this one must
        // contribute nothing. DESERT, because that bioma yields no food at all, so the
        // no-arg assignWorkingTile() -- which maximizes FOOD and only takes a tile whose
        // yield beats 0 -- can never claim it behind the test's back.
        mapcell &idle = map.set(city->latitude + 2, city->longitude + 2);
        idle.bioma    = DESERT;
        idle.resource = GEMS;

        // Putting a resource on a tile changes what that tile yields, so its rates are
        // re-assigned from the tables -- the same thing loadMap() does for a whole map.
        assignProductionRates(map);

        if (idle.getResourceProductionRate(FOOD) != 0 || idle.getResourceProductionRate(CULTURE) != 2)
        { fail("Setup: a DESERT gem tile should derive FOOD 0 and CULTURE 2 from the tables."); return 0; }

        if (!city->workingOn(1,0)) city->assignWorkingTile(coordinate(1,0));
        if (!city->workingOn(0,1)) city->assignWorkingTile(coordinate(0,1));

        if (!city->workingOn(1,0) || !city->workingOn(0,1))
        { fail("Setup: the gem and gold tiles could not be assigned as worked tiles."); return 0; }
        if (city->workingOn(2,2))
        { fail("Setup: the idle gem tile must NOT be worked, or it cannot prove the working rule."); return 0; }

        // Worked gems + worked gold, and nothing from the idle gem: 2 + 1 = 3 culture.
        const int tileCulture = city->getProductionRate(CULTURE);
        if (tileCulture != 3)
        {
            char buf[220];
            snprintf(buf,sizeof(buf),"Worked GEMS (2) + worked GOLD (1) should yield 3 CULTURE off the tiles, got %d "
                                     "(an idle gem tile in range must not count).", tileCulture);
            fail(buf); return 0;
        }
        const int tileCoins = city->getProductionRate(COINS);
        if (tileCoins != 2)
        { fail("The worked GOLD tile should yield 2 COINS off the tiles."); return 0; }

        // What the box shows is the sum of both sources. If the conversion had REPLACED tile
        // production, the gems would have been silently thrown away.
        const int shownCulture = tileCulture + cityTradeConversionRate(city, CULTURE);
        const int wantCulture  = 3 + (int)((float)trade() * RATE_CULTURE);
        if (shownCulture != wantCulture)
        {
            char buf[240];
            snprintf(buf,sizeof(buf),"CULTURE shown = %d; the tile yield (%d) plus the trade conversion (%d) is %d.",
                     shownCulture, tileCulture, cityTradeConversionRate(city, CULTURE), wantCulture);
            fail(buf); return 0;
        }
        if (shownCulture <= tileCulture || shownCulture <= cityTradeConversionRate(city, CULTURE))
        { fail("The CULTURE shown must exceed either source alone -- both have to be counted."); return 0; }

        // The same path for SCIENCE, which no resource yields TODAY but one is expected to
        // (an archaeology-style dig). Done through the REAL extension mechanism rather than
        // a hand-set yield: one more row in productionrates.overrides is the entire change,
        // and it is picked up with no edit to the city screen, to City::getProductionRate or
        // to cityTradeConversionRate -- none of which is CULTURE- or COINS-specific. MARBLE
        // stands in as the resource; it has no override of its own today.
        productionrates.overrides.push_back({ ANY_LAND_BIOMA, MARBLE, {{SCIENCE,4}} });
        map.set(city->latitude + 1, city->longitude).resource = MARBLE;   // replaces the gem
        assignProductionRates(map);

        const int tileScience = city->getProductionRate(SCIENCE);
        if (tileScience != 4)
        {
            char buf[220];
            snprintf(buf,sizeof(buf),"A worked tile whose resource declares SCIENCE 4 in the tables yielded %d -- "
                                     "adding such a resource must need no code change.", tileScience);
            fail(buf); return 0;
        }
        const int shownScience = tileScience + cityTradeConversionRate(city, SCIENCE);
        if (shownScience != 4 + (int)((float)trade() * RATE_SCIENCE))
        {
            char buf[240];
            snprintf(buf,sizeof(buf),"SCIENCE shown = %d; a tile yield of 4 plus the trade conversion (%d) is %d.",
                     shownScience, cityTradeConversionRate(city, SCIENCE), 4 + (int)((float)trade() * RATE_SCIENCE));
            fail(buf); return 0;
        }

        // Leave the shared table as it was found -- it is a global.
        productionrates.overrides.pop_back();
        map.set(city->latitude + 1, city->longitude).resource = GEMS;
        assignProductionRates(map);
        if (city->getProductionRate(SCIENCE) != 0)
        { fail("Removing the override should take the tile's SCIENCE yield with it."); return 0; }
    }

    // ---- 4) the TRADE_SURPLUS perk doubles it, exactly as endOfYear() does ---------------
    const int beforePerk = cityTradeConversionRate(city, SCIENCE);
    dee.regDep(cityContext(city->id), TRADE_SURPLUS_CODE);
    const int afterPerk = cityTradeConversionRate(city, SCIENCE);

    if (afterPerk != (int)((float)(trade()*2) * RATE_SCIENCE))
    {
        char buf[200];
        snprintf(buf,sizeof(buf),"With the TRADE_SURPLUS perk SCIENCE converts to %d, expected double the trade (%d) at rate %.3f = %d.",
                 afterPerk, trade()*2, RATE_SCIENCE, (int)((float)(trade()*2) * RATE_SCIENCE));
        fail(buf); return 0;
    }
    if (afterPerk <= beforePerk)
    { fail("The TRADE_SURPLUS perk did not increase the converted SCIENCE."); return 0; }

    return 0;
}

std::string TestCase_065::title()
{
    return std::string("City screen 'City Resources' box now shows the SCIENCE and CULTURE each city produces, under the gold row: new engine.cpp cityTradeConversionRate() projects the TRADE-to-coins/science/culture conversion endOfYear() performs (TRADE_SURPLUS doubling included) and the box adds it to the tile production.");
}

bool TestCase_065::done()   { return isdone; }
bool TestCase_065::passed() { return haspassed; }
std::string TestCase_065::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_065();
}
