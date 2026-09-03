//  TestCase_046.cpp
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
#include "../units/Galleon.h"
#include "../units/Transport.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../cityscreenui.h"

#include "testcase_046.h"

// @Task: "Extend the number of cargo boxes in ships to 6" (Galleon). The model layer already
// scales -- each Transport reports its own capacity() (Galleon 6, Trireme/Wagon 2) and
// Command::LoadCargoOrder already gates on Transport::board() -- so this exercises the city
// UI, which was the part hardcoded to 2:
//   1. Load 6 distinct commodities onto a Galleon through the "Resource Storage" load arrow;
//      all 6 must board (manifest()==6==capacity()), each deducting 100 from the city.
//   2. A 7th distinct commodity must fail to board cleanly (no city deduction, nothing
//      aboard) -- board() returned false, same "full transport" path testcase_042 pins for
//      the 2-slot Wagon.
//   3. drawCityScreen() renders the Units box with all 6 cargo slots full (and the Galleon's
//      name shifted right to make room) without crashing.
//   4. Unload via the cargo-slot click path for slots across the whole widened lon2 range --
//      slot 5 is at lon2==1 and slot 0 at lon2==-4 (slot s -> lon2 == s-4); with the old
//      `lon2==-4 -> slot 0 / lon2==-3 -> slot 1` handler only the first two slots could be
//      clicked at all.

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

// The six commodities loaded, in load order, plus a seventh that must not fit.
static const int LOADED[6] = { copper, iron, silver, marble, furs, traan };
static const int SEVENTH   = gems;

TestCase_046::TestCase_046() {}
TestCase_046::~TestCase_046() {}

int TestCase_046::number()
{
    return 46;
}

void TestCase_046::init()
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

    // Not at (0,0): same reasoning as testcase_042/044 -- drawCityScreen is normally called
    // with the city's real screen position, so (0,0) masks code that forgets to add cla/clo.
    City *city = new City(&map, 0, getNextCityId(), 3, 3);
    city->setName("Kattegate");
    city->foundedyear = -4000;
    for (int c_id : LOADED) city->commodities[c_id] = 250;
    city->commodities[SEVENTH] = 250;
    cities[city->id] = city;
    cityid = city->id;

    citynames[0] = std::queue<std::string>();

    Galleon *galleon = new Galleon();
    galleon->id = getNextUnitId();
    galleon->faction = 0;
    galleon->latitude = 3;
    galleon->longitude = 3;
    galleon->availablemoves = galleon->getUnitMoves();
    units[galleon->id] = galleon;
    galleonid = galleon->id;

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = galleonid;
}

int TestCase_046::check(int year)
{
    ticks++;

    if (isdone)
        return 0;

    // tester.cpp resets controller.view=1 right after init() -- re-set it every tick.
    controller.view = 2;
    controller.cityid = cityid;

    if (ticks < 3)
        return 0;

    City* city = cities[cityid];
    Transport* transport = dynamic_cast<Transport*>(units[galleonid]);

    if (transport->capacity() != 6)
    {
        isdone = true; haspassed = false;
        char buf[128];
        snprintf(buf,sizeof(buf),"Galleon capacity() is %d, expected 6.", transport->capacity());
        message = std::string(buf);
        return 0;
    }

    // The "Resource Storage" box shows only COMMODITIES_STORAGE_ROWS (5) rows at a time, so
    // reaching the 6th/7th stocked resource means scrolling it down first (the down arrow is
    // at lat2==18,lon2==-8, and decrements the box's offset). `off` mirrors that offset so
    // the test can work out which on-screen row (lat 5..9) a given absolute list index is at.
    // getStockedResources() lists commodities in ALL_COMMODITIES order, so LOADED[k] is at
    // absolute index k and SEVENTH at index 6.
    int off = 0;
    auto scrollTo = [&](int wantOff){ while (off > wantOff) { clickOnCityScreen(0,0,18,-8); off--; } };
    auto rowFor   = [&](int absIndex){ return 5 + (absIndex + off); };

    // 1) Load all six commodities, one cargo slot each.
    for (int k=0;k<6;k++)
    {
        int r_id = LOADED[k];
        scrollTo(k <= 4 ? 0 : -(k-4));      // first 5 fit at offset 0; then scroll one per extra row
        int lat = rowFor(k);
        if (lat < 5 || lat > 9)
        {
            isdone = true; haspassed = false;
            char buf[128];
            snprintf(buf,sizeof(buf),"Row math off: commodity #%d maps to lat %d (off=%d).", k, lat, off);
            message = std::string(buf);
            return 0;
        }

        clickOnCityScreen(lat, -5, 0, 0);   // "load" arrow, column -5
        processCommandOrders();

        if (city->commodities[r_id] != 150)
        {
            isdone = true; haspassed = false;
            char buf[160];
            snprintf(buf,sizeof(buf),"Loading commodity #%d (id %d) did not deduct 100 (stock=%d).",
                     k, r_id, city->commodities[r_id]);
            message = std::string(buf);
            return 0;
        }
        Resource* r = dynamic_cast<Resource*>(transport->findCargo(r_id));
        if (r == nullptr || r->amount != 100)
        {
            isdone = true; haspassed = false;
            char buf[160];
            snprintf(buf,sizeof(buf),"Commodity #%d (id %d) not aboard the Galleon with amount 100.", k, r_id);
            message = std::string(buf);
            return 0;
        }
        if (transport->manifest() != k+1)
        {
            isdone = true; haspassed = false;
            char buf[128];
            snprintf(buf,sizeof(buf),"After loading #%d the Galleon manifest() is %d, expected %d.",
                     k, transport->manifest(), k+1);
            message = std::string(buf);
            return 0;
        }
    }

    // 2) Seventh distinct commodity (absolute index 6): Galleon is full, must fail cleanly.
    scrollTo(-2);
    clickOnCityScreen(rowFor(6), -5, 0, 0);
    processCommandOrders();

    if (city->commodities[SEVENTH] != 250 || transport->findCargo(SEVENTH) != nullptr || transport->manifest() != 6)
    {
        isdone = true; haspassed = false;
        char buf[160];
        snprintf(buf,sizeof(buf),"Loading a 7th commodity onto a full Galleon was not rejected cleanly (stock=%d, manifest=%d).",
                 city->commodities[SEVENTH], transport->manifest());
        message = std::string(buf);
        return 0;
    }

    // 3) Sanity render: Units box with all 6 cargo slots full, at the city's real position.
    coordinate c = map.to_screen(city->latitude, city->longitude);
    drawCityScreen(c.lat, c.lon, city);

    // 4) Unload via the cargo-slot click path, at both ends of the widened lon2 range.
    //    Slot s is clicked as lon2 == s-4, so slot 5 -> lon2==1 and slot 0 -> lon2==-4.
    for (int slot : { 5, 0 })
    {
        std::vector<Shippable*> cargo = transport->getCargo();
        int before = transport->manifest();
        int r_id = cargo[slot]->getId();

        clickOnCityScreen(5, 0, 0, slot - 4);   // Galleon is the only unit here -> row lat==5
        processCommandOrders();

        if (city->commodities[r_id] != 250 || transport->findCargo(r_id) != nullptr || transport->manifest() != before-1)
        {
            isdone = true; haspassed = false;
            char buf[160];
            snprintf(buf,sizeof(buf),"Unloading slot %d (lon2=%d, id %d) failed: stock=%d, manifest=%d (expected %d).",
                     slot, slot-4, r_id, city->commodities[r_id], transport->manifest(), before-1);
            message = std::string(buf);
            return 0;
        }
    }

    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_046::title()
{
    return std::string("Galleon cargo: 6 cargo slots in the city UI (load 6, reject the 7th, unload across the widened lon2 range).");
}

bool TestCase_046::done()    { return isdone; }
bool TestCase_046::passed()  { return haspassed; }
std::string TestCase_046::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_046();
}
