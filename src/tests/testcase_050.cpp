//  TestCase_050.cpp
//  bunmei
//
//  Created by Claude on 04/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <algorithm>

#include "../map.h"
#include "../units/Unit.h"
#include "../units/Trireme.h"
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

#include "testcase_050.h"

// @Issue: a Transport docked at a foreign city to trade could only ever buy ONE 100-unit
// stack of a given resource, even with a free cargo slot left -- Command::BuyResourceOrder
// (and the plain LoadCargoOrder it mirrors) topped up "the" existing stack of that resource
// id if one was aboard, and did nothing more once it hit the 100 cap, never trying a second
// slot. Root cause: Transport::board() stored cargo in an unordered_map KEYED BY RESOURCE
// ID, so a second stack of the same resource could not even coexist with the first (it would
// have silently overwritten the map entry). Fixed by switching Wagon/Trireme/Galleon's cargo
// storage to a slot-ordered std::vector, and engine.cpp's Load/BuyResourceOrder to look for a
// NOT-YET-FULL stack of the resource (engine.cpp:findToppableCargo) rather than assuming at
// most one stack of a given resource can ever be aboard.
//
// Reproduces the reported scenario: a Trireme (capacity 2) docks at a city holding 230
// elephants -- buying should fill 100 into slot 1, then another 100 into slot 2.

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

TestCase_050::TestCase_050() {}
TestCase_050::~TestCase_050() {}

int TestCase_050::number()
{
    return 50;
}

void TestCase_050::init()
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
    factions.push_back(f0);

    Faction *f1 = new Faction();
    f1->id = 1; strcpy(f1->name,"Romans");
    f1->red = 0; f1->green = 0; f1->blue = 255;
    f1->autoPlayer = true;
    factions.push_back(f1);

    initDiplomacy(diplomacy, 2);
    diplomacy[0][1].makePeace();

    // Buyer's treasury (capital city, faction 0).
    City *home = new City(&map, 0, getNextCityId(), 5, 1);
    home->setName("Kaupang");
    home->foundedyear = -4000;
    home->setCapitalCity();
    home->resources[COINS] = 1000;
    cities[home->id] = home;
    homeid = home->id;

    // Foreign city with 230 elephants for sale.
    City *city = new City(&map, 1, getNextCityId(), 5, 5);
    city->setName("Roma");
    city->foundedyear = -4000;
    city->resources[elephants] = 230;
    city->resources[COINS] = 0;
    cities[city->id] = city;
    cityid = city->id;

    citynames[0] = std::queue<std::string>();
    citynames[1] = std::queue<std::string>();

    Trireme *t = new Trireme();
    t->id = getNextUnitId();
    t->faction = 0;
    t->latitude = 5;
    t->longitude = 4;
    t->availablemoves = t->getUnitMoves();
    units[t->id] = t;
    triremeid = t->id;

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.v_f_id = 0;
}

int TestCase_050::check(int year)
{
    ticks++;
    if (isdone) return 0;
    controller.view = 4;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    City* city = cities[cityid];
    City* home = cities[homeid];
    Unit* triremeU = units[triremeid];
    Transport* trireme = dynamic_cast<Transport*>(triremeU);

    if (trireme->capacity() != 2)
    {
        fail("Setup: expected a 2-slot Transport (Trireme).");
        return 0;
    }

    if (!engageTrade(triremeU, 5, 5))
    {
        fail("engageTrade rejected the Trireme at the foreign PEACE city.");
        return 0;
    }

    // Row 0 of "For sale" (elephants is the only stock) -> lat 5, buy arrow column -5,
    // same layout clickOnCommerceScreen/drawResourceStorageBox use.
    auto buy = [&](){ clickOnCommerceScreen(5, -5, 0, 0); processCommandOrders(); };

    // 1) First buy: slot 1 gets 100 elephants.
    buy();
    if (trireme->manifest() != 1)
    {
        char buf[128];
        snprintf(buf,sizeof(buf),"After 1st buy manifest()=%d, expected 1.", trireme->manifest());
        fail(buf); return 0;
    }
    {
        Resource* r = dynamic_cast<Resource*>(trireme->findCargo(elephants));
        if (r == nullptr || r->amount != 100)
        {
            fail("After 1st buy, slot 1 does not hold 100 elephants.");
            return 0;
        }
    }
    if (city->resources[elephants] != 130 || home->resources[COINS] != 900 || city->resources[COINS] != 100)
    {
        char buf[200];
        snprintf(buf,sizeof(buf),"After 1st buy bad totals (Roma elephants %d exp 130, Kaupang COINS %d exp 900, Roma COINS %d exp 100).",
                 city->resources[elephants], home->resources[COINS], city->resources[COINS]);
        fail(buf); return 0;
    }

    // 2) Second buy: the ONLY existing stack is already full (100), so this MUST open a
    //    second cargo slot instead of silently doing nothing -- the reported bug.
    buy();
    if (trireme->manifest() != 2)
    {
        char buf[128];
        snprintf(buf,sizeof(buf),"After 2nd buy manifest()=%d, expected 2 (a second elephants stack should now be aboard).", trireme->manifest());
        fail(buf); return 0;
    }
    if (city->resources[elephants] != 30 || home->resources[COINS] != 800 || city->resources[COINS] != 200)
    {
        char buf[200];
        snprintf(buf,sizeof(buf),"After 2nd buy bad totals (Roma elephants %d exp 30, Kaupang COINS %d exp 800, Roma COINS %d exp 200).",
                 city->resources[elephants], home->resources[COINS], city->resources[COINS]);
        fail(buf); return 0;
    }
    // Both slots hold their own independent 100-unit stack.
    std::vector<Shippable*> cargo = trireme->getCargo();
    if (cargo.size() != 2 || cargo[0]->getId() != elephants || cargo[1]->getId() != elephants
        || dynamic_cast<Resource*>(cargo[0])->amount != 100 || dynamic_cast<Resource*>(cargo[1])->amount != 100)
    {
        fail("getCargo() does not show two independent 100-unit elephants stacks in slots 0 and 1.");
        return 0;
    }

    // 3) Third buy: Trireme is full (2/2 slots, both stacks at the 100 cap) -> a genuine
    //    no-op, must not silently create a third stack or touch any totals.
    buy();
    if (trireme->manifest() != 2 || city->resources[elephants] != 30
        || home->resources[COINS] != 800 || city->resources[COINS] != 200)
    {
        fail("Third buy changed state -- a full Transport must reject the purchase entirely.");
        return 0;
    }

    // 4) Sell slot 0's stack via the "Port" box.png click (fine grid lon2 == slot-4 == -4):
    //    only ONE stack comes off, the other 100 stays aboard.
    clickOnCommerceScreen(5, 0, 0, -4);
    processCommandOrders();
    if (trireme->manifest() != 1)
    {
        char buf[128];
        snprintf(buf,sizeof(buf),"After selling one slot manifest()=%d, expected 1 (the other stack must remain).", trireme->manifest());
        fail(buf); return 0;
    }
    if (city->resources[elephants] != 130 || home->resources[COINS] != 900 || city->resources[COINS] != 100)
    {
        fail("Selling one elephants stack did not restore the expected totals.");
        return 0;
    }

    // Sanity render with two (then one) independent stacks of the same resource aboard --
    // must not crash (exercises drawUnitsBoxRow's per-slot box.png loop).
    controller.cityid = cityid;
    openCommerceScreen();

    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_050::title()
{
    return std::string("A Transport can buy several separate 100-unit stacks of the SAME resource across its free cargo slots (was capped at one stack total).");
}

bool TestCase_050::done()   { return isdone; }
bool TestCase_050::passed() { return haspassed; }
std::string TestCase_050::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_050();
}
