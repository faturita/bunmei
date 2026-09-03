//  TestCase_044.cpp
//  bunmei
//
//  Created by Claude on 01/09/2026
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
#include "../units/Transport.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../cityscreenui.h"

#include "testcase_044.h"

// @Task: "Adding MfgGoods to city UI" -- the left-bottom box (renamed "Resource Storage")
// now lists stocked mfg goods after the stocked commodities, and mfg goods can be shipped
// on a Wagon/Trireme through the same "load" arrow the commodities use.
//
// A rendered box can't be inspected by a testcase, so this exercises the two pieces the UI
// is built on directly:
//   1. getStockedResources() -- the row order drawCityScreen()/clickOnCityScreen() share:
//      stocked commodities first (ALL_COMMODITIES order), then stocked mfg goods
//      (ALL_MFG_GOODS order).
//   2. The icon-strip quantity: one resource icon per 10 units (floor), capped at 30 icons
//      for the 300-per-city ceiling.
//   3. The real cargo path for a mfg good: clicking the "load" arrow on a mfg-good row
//      pushes Command::LoadCargoOrder, and processCommandOrders() moves it onto the Wagon
//      -- then an unload via the Units-box cargo slot puts it back. This also pins the
//      engine.cpp classification fix (ismfggood = resourceid >= rum, not >= tools): with
//      the old `>= tools` test, rum (0x301) counted as a commodity and nothing loaded.
// Plus a sanity drawCityScreen() render with mfg goods in storage and aboard (must not
// crash -- exercises the new tiles[] mfg-good icon lookups).

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

// Mirrors the strip math in cityscreenui.cpp drawCityScreen(): floor(amount/10), capped.
#define RESOURCE_STRIP_SLOTS 30
static int stripIcons(int amount)
{
    int icons = amount/10;
    return icons > RESOURCE_STRIP_SLOTS ? RESOURCE_STRIP_SLOTS : icons;
}

TestCase_044::TestCase_044()
{

}

TestCase_044::~TestCase_044()
{

}

int TestCase_044::number()
{
    return 44;
}

void TestCase_044::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon) = mapcell(LAND);

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

    // Not at (0,0): same reasoning as testcase_042 -- drawCityScreen is normally called with
    // the city's real (nonzero) screen position, and a city at true (0,0) masks any drawing
    // code that forgets to add cla/clo to its own coordinates.
    City *city = new City(&map, 0, getNextCityId(), 3, 3);
    city->setName("Kattegate");
    city->foundedyear = -4000;
    city->commodities[copper] = 143;   // -> 14 strip icons
    city->commodities[iron]   = 7;     // -> 0 strip icons (below one full icon)
    city->mfggoods[rum]       = 250;   // -> 25 strip icons
    city->mfggoods[tools]     = 90;    // -> 9 strip icons
    cities[city->id] = city;
    cityid = city->id;

    citynames[0] = std::queue<std::string>();

    Wagon *wagon = new Wagon();
    wagon->id = getNextUnitId();
    wagon->faction = 0;
    wagon->latitude = 3;
    wagon->longitude = 3;
    wagon->availablemoves = wagon->getUnitMoves();
    units[wagon->id] = wagon;
    wagonid = wagon->id;

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = wagonid;
}

int TestCase_044::check(int year)
{
    ticks++;

    if (isdone)
        return 0;

    // tester.cpp resets controller.view=1 right after init() -- re-set it every tick (the
    // testcase_025/029/042 workaround) so the city screen is actually open for the render.
    controller.view = 2;
    controller.cityid = cityid;

    if (ticks < 3)
        return 0;

    City* city = cities[cityid];
    Transport* transport = dynamic_cast<Transport*>(units[wagonid]);

    // 1) Row order: commodities (ALL_COMMODITIES order) then mfg goods (ALL_MFG_GOODS order).
    std::vector<int> stocked = getStockedResources(city);
    std::vector<int> expected = { (int)copper, (int)iron, (int)rum, (int)tools };
    if (stocked != expected)
    {
        isdone = true; haspassed = false;
        char buf[256];
        snprintf(buf,sizeof(buf),
            "getStockedResources() order wrong: got %zu entries, expected copper,iron,rum,tools.",
            stocked.size());
        message = std::string(buf);
        return 0;
    }

    // 2) Icon-strip quantity: floor(amount/10), capped at 30.
    struct { int amount; int icons; } stripcases[] = {
        {143, 14}, {7, 0}, {250, 25}, {90, 9}, {300, 30}, {450, 30},
    };
    for (auto& sc : stripcases)
    {
        if (stripIcons(sc.amount) != sc.icons)
        {
            isdone = true; haspassed = false;
            char buf[256];
            snprintf(buf,sizeof(buf),"strip icons for amount %d was %d, expected %d.",
                     sc.amount, stripIcons(sc.amount), sc.icons);
            message = std::string(buf);
            return 0;
        }
    }

    // 3) Load a mfg good (rum) onto the Wagon through the real UI click path.
    auto rumit = std::find(stocked.begin(), stocked.end(), (int)rum);
    int rumlat = 5 + (int)(rumit - stocked.begin());

    clickOnCityScreen(rumlat, -5, 0, 0);   // "load" arrow, column -5
    processCommandOrders();

    if (city->mfggoods[rum] != 150)
    {
        isdone = true; haspassed = false;
        // With the old `ismfggood = resourceid >= tools` test this fails: rum is misread as
        // a commodity, city->commodities[rum] (0) is drawn from, and nothing is loaded.
        char buf[256];
        snprintf(buf,sizeof(buf),"Loading rum did not deduct 100 from the city (mfggoods[rum]=%d, expected 150).",
                 city->mfggoods[rum]);
        message = std::string(buf);
        return 0;
    }

    Shippable* aboard = transport->findCargo(rum);
    Resource* aboardr = dynamic_cast<Resource*>(aboard);
    if (aboard == nullptr || aboardr == nullptr || aboardr->amount != 100)
    {
        isdone = true; haspassed = false;
        message = std::string("Rum is not aboard the Wagon with amount 100 after loading.");
        return 0;
    }

    if (city->commodities[rum] != 0)
    {
        isdone = true; haspassed = false;
        message = std::string("Rum leaked into city->commodities -- mfg goods must stay in city->mfggoods.");
        return 0;
    }

    // Sanity render with a mfg good in storage AND aboard the Wagon: must not crash.
    coordinate c = map.to_screen(city->latitude, city->longitude);
    drawCityScreen(c.lat, c.lon, city);

    // 4) Unload rum back into the city via its cargo slot in the Units box (rum is the only
    // cargo -> slot 0 -> lon2 = -4, same mapping testcase_042 uses).
    clickOnCityScreen(5, 0, 0, -4);
    processCommandOrders();

    isdone = true;
    haspassed = (city->mfggoods[rum] == 250 && transport->findCargo(rum) == nullptr && transport->manifest() == 0);
    if (!haspassed)
    {
        char buf[256];
        snprintf(buf,sizeof(buf),"Unloading rum did not restore the city (mfggoods[rum]=%d) / empty the Wagon.",
                 city->mfggoods[rum]);
        message = std::string(buf);
    }

    return 0;
}

std::string TestCase_044::title()
{
    return std::string("Resource Storage box: mfg goods listed after commodities (getStockedResources) and shippable on a Wagon like commodities.");
}

bool TestCase_044::done()
{
    return isdone;
}
bool TestCase_044::passed()
{
    return haspassed;
}
std::string TestCase_044::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_044();
}
