//  TestCase_042.cpp
//  bunmei
//
//  Created by Claude on 29/08/2026
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

#include "testcase_042.h"

// @Task: "Allow Transport classes (now only Trireme and Wagon) to ship Resources
// (Commodity and MfgGoods)". Exercises the whole city-UI cargo flow end to end, through the
// real production code path (clickOnCityScreen pushes a CommandOrder, processCommandOrders()
// is what actually moves the resource) rather than calling the resource-transfer logic
// directly:
//   1. Click the Commodities Storage box's "load" arrow (cursor/right.png, column -5) for
//      two different commodities onto a Wagon stationed at the city -- each becomes its own
//      cargo slot (Wagon/Trireme capacity is 2), capped at 100 units.
//   2. Clicking the SAME commodity's arrow again, once its boarded stack is already at the
//      100 cap, must NOT take a second slot or move any more of it out of the city.
//   3. A third, different commodity, once both cargo slots are already taken, must fail to
//      board -- the city's stock for it must stay untouched (no partial deduction) and
//      nothing crashes (this is exactly the path that used to leak the new Commodity object
//      on a full ship before the fix).
//   4. Clicking a loaded cargo slot's box.png icon in the Units box (lon2==-4/-3, see
//      drawCityScreen's own comment on that derivation) unloads it back into the city,
//      restoring the city's stock and freeing the slot.
// Also renders the city screen once midway through (with cargo actually aboard, in both
// slots) as a crash/sanity check on the new box.png + resource-icon-overlay drawing code.

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

TestCase_042::TestCase_042()
{

}

TestCase_042::~TestCase_042()
{

}

int TestCase_042::number()
{
    return 42;
}

void TestCase_042::init()
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

    // Deliberately NOT at (0,0): drawCityScreen(cla,clo,...) is normally called with the
    // city's actual (nonzero) screen position (map.cpp:openCityScreen), never (0,0) -- a
    // city placed at true map (0,0) would silently mask any drawing code that forgets to add
    // cla/clo to its own coordinates (exactly what happened to the cargo slot icons: task #35
    // shipped with `int x = slot*8-32;`, missing `clo*16`, and this test originally placed
    // the city at (0,0) too, so it never caught it).
    City *city = new City(&map, 0, getNextCityId(), 3, 3);
    city->setName("Kattegate");
    city->foundedyear = -4000;
    city->commodities[copper] = 250;
    city->commodities[iron] = 250;
    city->commodities[silver] = 250;
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

int TestCase_042::check(int year)
{
    ticks++;

    if (isdone)
        return 0;

    // tester.cpp's initWorldModelling() runs right after init() and unconditionally resets
    // controller.view=1 -- setting view=2/cityid here (every tick, not just once) is the
    // established workaround (testcase_025/026/029) for a test that needs the city screen
    // actually open, rather than in init() where it would just get clobbered.
    controller.view = 2;
    controller.cityid = cityid;

    // Give the game loop a few ticks to settle, same as testcase_025/026/029/032.
    if (ticks < 3)
        return 0;

    City* city = cities[cityid];
    Transport* transport = dynamic_cast<Transport*>(units[wagonid]);

    std::vector<int> stocked = getStockedResources(city);
    auto copperit = std::find(stocked.begin(), stocked.end(), (int)copper);
    auto ironit   = std::find(stocked.begin(), stocked.end(), (int)iron);
    auto silverit = std::find(stocked.begin(), stocked.end(), (int)silver);
    if (copperit==stocked.end() || ironit==stocked.end() || silverit==stocked.end())
    {
        isdone = true; haspassed = false;
        message = std::string("Test setup broken: copper/iron/silver not all in getStockedResources().");
        return 0;
    }
    int copperlat = 5 + (int)(copperit-stocked.begin());
    int ironlat   = 5 + (int)(ironit-stocked.begin());
    int silverlat = 5 + (int)(silverit-stocked.begin());

    {
        // 1) Load copper: new cargo slot, capped at 100.
        clickOnCityScreen(copperlat, -5, 0, 0);
        processCommandOrders();

        if (city->commodities[copper] != 150)
        {
            isdone = true; haspassed = false;
            message = std::string("Loading copper did not deduct 100 from the city's stock.");
            return 0;
        }
        Resource* r = dynamic_cast<Resource*>(transport->findCargo(copper));
        if (r == nullptr || r->amount != 100)
        {
            isdone = true; haspassed = false;
            message = std::string("Copper is not aboard the Wagon with amount 100 after loading.");
            return 0;
        }

        // 2) Load iron: second (and last) cargo slot.
        clickOnCityScreen(ironlat, -5, 0, 0);
        processCommandOrders();

        if (city->commodities[iron] != 150)
        {
            isdone = true; haspassed = false;
            message = std::string("Loading iron did not deduct 100 from the city's stock.");
            return 0;
        }
        r = dynamic_cast<Resource*>(transport->findCargo(iron));
        if (r == nullptr || r->amount != 100)
        {
            isdone = true; haspassed = false;
            message = std::string("Iron is not aboard the Wagon with amount 100 after loading.");
            return 0;
        }

        // Sanity render with cargo actually aboard (both slots full), using the city's REAL
        // (nonzero) screen position -- same as map.cpp:openCityScreen() -- not (0,0): must
        // not crash. (This is also why the city/Wagon above are placed at (3,3), not (0,0):
        // drawing code that forgets to add cla/clo to its own coordinates, like the cargo
        // slot icons originally did, is invisible at (0,0) but breaks at any real position.)
        coordinate c = map.to_screen(city->latitude, city->longitude);
        drawCityScreen(c.lat, c.lon, city);
    }

    // 3) Re-click copper's arrow: already at the 100 cap, must be a no-op.
    clickOnCityScreen(copperlat, -5, 0, 0);
    processCommandOrders();

    if (city->commodities[copper] != 150)
    {
        isdone = true; haspassed = false;
        message = std::string("Re-loading an already-capped commodity changed the city's stock.");
        return 0;
    }
    Resource* r = dynamic_cast<Resource*>(transport->findCargo(copper));
    if (r == nullptr || r->amount != 100)
    {
        isdone = true; haspassed = false;
        message = std::string("Re-loading an already-capped commodity changed its boarded amount.");
        return 0;
    }

    // 4) Both slots are taken: loading a third, different commodity must fail cleanly.
    clickOnCityScreen(silverlat, -5, 0, 0);
    processCommandOrders();

    if (city->commodities[silver] != 250)
    {
        isdone = true; haspassed = false;
        message = std::string("Loading onto a full Transport still deducted from the city's stock.");
        return 0;
    }
    if (transport->findCargo(silver) != nullptr)
    {
        isdone = true; haspassed = false;
        message = std::string("Silver ended up aboard a Transport that was already at capacity.");
        return 0;
    }

    // 5) Unload copper via its cargo slot icon in the Units box -- the Wagon is the only
    // unit stationed here, so its row is lat==5. Ask the Transport itself which slot (0/1)
    // copper is currently in, same as drawCityScreen does every frame, rather than assuming
    // an iteration order.
    std::vector<Shippable*> cargo = transport->getCargo();
    int copperslot = -1;
    for (int i=0;i<(int)cargo.size();i++)
        if (cargo[i]->getId()==copper) copperslot = i;
    if (copperslot < 0)
    {
        isdone = true; haspassed = false;
        message = std::string("Copper is not found in getCargo() right before the unload click.");
        return 0;
    }
    clickOnCityScreen(5, 0, 0, copperslot==0 ? -4 : -3);
    processCommandOrders();

    if (city->commodities[copper] != 250)
    {
        isdone = true; haspassed = false;
        message = std::string("Unloading copper did not restore the city's stock.");
        return 0;
    }
    if (transport->findCargo(copper) != nullptr)
    {
        isdone = true; haspassed = false;
        message = std::string("Copper is still aboard the Wagon after unloading.");
        return 0;
    }

    // 6) Unload iron the same way (re-querying the slot: with copper gone, iron may now be
    // reported at a different index than before).
    cargo = transport->getCargo();
    int ironslot = -1;
    for (int i=0;i<(int)cargo.size();i++)
        if (cargo[i]->getId()==iron) ironslot = i;
    if (ironslot < 0)
    {
        isdone = true; haspassed = false;
        message = std::string("Iron is not found in getCargo() right before the unload click.");
        return 0;
    }
    clickOnCityScreen(5, 0, 0, ironslot==0 ? -4 : -3);
    processCommandOrders();

    isdone = true;
    haspassed = (city->commodities[iron]==250 && transport->findCargo(iron)==nullptr && transport->manifest()==0);
    if (!haspassed)
        message = std::string("Unloading iron did not restore the city's stock / empty the Wagon.");

    return 0;
}
std::string TestCase_042::title()
{
    return std::string("Transport cargo: loading/unloading Commodities through the city UI (Command::LoadCargoOrder/UnloadCargoOrder).");
}

bool TestCase_042::done()
{
    return isdone;
}
bool TestCase_042::passed()
{
    return haspassed;
}
std::string TestCase_042::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_042();
}
