//  TestCase_048.cpp
//  bunmei
//
//  Created by Claude on 03/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <deque>
#include <iterator>
#include <iostream>
#include <algorithm>

#include "../map.h"
#include "../units/Unit.h"
#include "../units/Wagon.h"
#include "../units/Warrior.h"
#include "../units/Transport.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../diplomacy.h"
#include "../usercontrols.h"
#include "../commerceui.h"

#include "testcase_048.h"

// @Task: "Commerce UI". A human Transport stepping onto a foreign city's tile at PEACE or
// better opens a commerce screen; the player buys from the city (pay faction->coins ->
// city->coreresources[COINS], resource boards the Transport) and sells cargo back (reverse).
// Prices live in a new global `prices` (tiles.cpp), all seeded to 1 by initPrices().
//
// Rendering can't be inspected by a testcase, so this drives the pieces underneath:
//   1. initPrices() seeds every commodity + mfg good at 1.
//   2. engageTrade() (engine.cpp, in the moveUnit() chain) accepts a human Transport at a
//      foreign PEACE+ city and sets controller.view/cityid/tradeunitid; rejects a
//      non-Transport and an own-faction city.
//   3. The commerce screen's left-box buy arrow -> Command::BuyResourceOrder: up to 100 units
//      move city -> Transport, treasury COINS -= qty*price, city->coreresources[COINS] += same.
//   4. Pressing a right-box box.png cargo slot -> Command::SellResourceOrder: the reverse.
//   5. A buy is capped by what the faction can afford.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Tiles tiles;
extern std::unordered_map<int, int> prices;
extern DiplomacyTable diplomacy;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_048::TestCase_048() {}
TestCase_048::~TestCase_048() {}

int TestCase_048::number()
{
    return 48;
}

void TestCase_048::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initPrices(prices);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon) = mapcell(LAND);
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon).setVisible(0);

    Faction *f0 = new Faction();
    f0->id = 0; strcpy(f0->name,"Vikings");
    f0->red = 255; f0->green = 0; f0->blue = 0;
    f0->autoPlayer = false;
    f0->coins = 500;
    factions.push_back(f0);

    Faction *f1 = new Faction();
    f1->id = 1; strcpy(f1->name,"Romans");
    f1->red = 0; f1->green = 0; f1->blue = 255;
    f1->autoPlayer = true;
    f1->coins = 0;
    factions.push_back(f1);

    initDiplomacy(diplomacy, 2);
    diplomacy[0][1].makePeace();          // PEACE (status 4) -> trade allowed

    // The buyer faction (0) needs a capital city -- its coreresources[COINS] is the
    // treasury trade pays from/into (there is no persistent Faction::coins pot).
    City *home = new City(&map, 0, getNextCityId(), 5, 1);
    home->setName("Kaupang");
    home->foundedyear = -4000;
    home->setCapitalCity();
    home->coreresources[COINS] = 500;
    cities[home->id] = home;
    homeid = home->id;

    City *city = new City(&map, 1, getNextCityId(), 5, 5);   // foreign city (faction 1)
    city->setName("Roma");
    city->foundedyear = -4000;
    city->commodities[copper] = 250;
    city->coreresources[COINS] = 0;
    cities[city->id] = city;
    cityid = city->id;

    citynames[0] = std::queue<std::string>();
    citynames[1] = std::queue<std::string>();

    Wagon *wagon = new Wagon();
    wagon->id = getNextUnitId();
    wagon->faction = 0;
    wagon->latitude = 5;
    wagon->longitude = 4;
    wagon->availablemoves = wagon->getUnitMoves();
    units[wagon->id] = wagon;
    wagonid = wagon->id;

    Warrior *warrior = new Warrior();
    warrior->id = getNextUnitId();
    warrior->faction = 0;
    warrior->latitude = 5;
    warrior->longitude = 3;
    units[warrior->id] = warrior;
    warriorid = warrior->id;

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = wagonid;
}

int TestCase_048::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    City*  city    = cities[cityid];
    City*  home    = cities[homeid];
    Unit*  wagonU  = units[wagonid];
    Transport* wagon = dynamic_cast<Transport*>(wagonU);

    // 1) initPrices seeded every tradeable resource at 1.
    if (prices[copper] != 1 || prices[iron] != 1 || prices[tools] != 1 || prices[rum] != 1)
    {
        fail("initPrices did not seed all commodities/mfg goods at price 1.");
        return 0;
    }

    // 2) engageTrade rejects a non-Transport, and an own-faction city.
    if (engageTrade(units[warriorid], 5, 5))
    {
        fail("engageTrade accepted a non-Transport unit.");
        return 0;
    }
    city->faction = 0;
    if (engageTrade(wagonU, 5, 5))
    {
        fail("engageTrade accepted the unit's OWN city.");
        return 0;
    }
    city->faction = 1;

    // 2b) engageTrade accepts the human Transport at the foreign PEACE city.
    if (!engageTrade(wagonU, 5, 5))
    {
        fail("engageTrade rejected a human Transport at a foreign PEACE city.");
        return 0;
    }
    if (controller.view != 4 || controller.cityid != cityid || controller.tradeunitid != wagonid
        || wagonU->availablemoves != 0)
    {
        fail("engageTrade did not set up the commerce screen (view/cityid/tradeunitid/moves).");
        return 0;
    }

    // Let the real render path (drawScene -> drawMap -> openCommerceScreen with view==4)
    // draw the commerce screen for a few frames before poking buy/sell -- a crash/sanity
    // pass on drawCommerceScreen / drawResourceStorageBox / drawUnitsBoxRow.
    if (t0 < 0)
    {
        t0 = ticks;
        return 0;
    }
    if (ticks < t0 + 5)
        return 0;

    // 3) Buy copper: "For sale" box row 0 -> lat 5, the per-row buy arrow at column -5
    //    (same layout as the city screen's Resource Storage box). Coins Kaupang -> Roma.
    clickOnCommerceScreen(5, -5, 0, 0);
    processCommandOrders();

    {
        Resource* r = dynamic_cast<Resource*>(wagon->findCargo(copper));
        if (r == nullptr || r->amount != 100)
        {
            fail("Buy: 100 copper did not board the Wagon.");
            return 0;
        }
        if (city->commodities[copper] != 150 || home->coreresources[COINS] != 400 || city->coreresources[COINS] != 100)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"Buy: bad totals (Roma copper %d exp 150, Kaupang COINS %d exp 400, Roma COINS %d exp 100).",
                     city->commodities[copper], home->coreresources[COINS], city->coreresources[COINS]);
            fail(buf);
            return 0;
        }
    }

    // 4) Sell it back: press the Wagon's box.png cargo slot in the "Port" box -- the
    //    Transport row is lat 5, copper is cargo slot 0 -> fine grid lon2 == 0-4 == -4
    //    (same slot mechanism as the city screen's Units box). Coins Roma -> Kaupang.
    clickOnCommerceScreen(5, 0, 0, -4);
    processCommandOrders();

    if (wagon->findCargo(copper) != nullptr || wagon->manifest() != 0)
    {
        fail("Sell: copper is still aboard the Wagon.");
        return 0;
    }
    if (city->commodities[copper] != 250 || home->coreresources[COINS] != 500 || city->coreresources[COINS] != 0)
    {
        char buf[200];
        snprintf(buf,sizeof(buf),"Sell: bad totals (Roma copper %d exp 250, Kaupang COINS %d exp 500, Roma COINS %d exp 0).",
                 city->commodities[copper], home->coreresources[COINS], city->coreresources[COINS]);
        fail(buf);
        return 0;
    }

    // 5) Affordability cap: Kaupang holds only 30 coins -> only 30 copper bought.
    home->coreresources[COINS] = 30;
    clickOnCommerceScreen(5, -5, 0, 0);
    processCommandOrders();

    {
        Resource* r = dynamic_cast<Resource*>(wagon->findCargo(copper));
        if (r == nullptr || r->amount != 30 || home->coreresources[COINS] != 0
            || city->commodities[copper] != 220 || city->coreresources[COINS] != 30)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"Affordability: expected 30 bought (aboard %d, Kaupang COINS %d, Roma copper %d, Roma COINS %d).",
                     r ? r->amount : -1, home->coreresources[COINS], city->commodities[copper], city->coreresources[COINS]);
            fail(buf);
            return 0;
        }
    }

    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_048::title()
{
    return std::string("Commerce UI: engageTrade opens the market at a foreign PEACE city; buy/sell move resources and COINS between faction and city, buy capped by affordability.");
}

bool TestCase_048::done()   { return isdone; }
bool TestCase_048::passed() { return haspassed; }
std::string TestCase_048::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_048();
}
