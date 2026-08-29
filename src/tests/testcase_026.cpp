//  TestCase_026.cpp
//  bunmei
//
//  Created by Claude on 27/07/2026
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

#include "../map.h"
#include "../units/Unit.h"
#include "../units/Warrior.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../map.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../cityscreenui.h"

#include "testcase_026.h"

// The city screen's "Units" box (task #7, testcase_025) only shows UNITS_BOX_ROWS (5) units
// at a time; with more than that stationed on a city's tile it now scrolls, via the SAME
// up/down-arrow mechanism (and the same look and feel) as the existing "Change" (buildable)
// list box (cityscreenui.cpp): clicking the down arrow (lat2==18,lon2==6, same row values as
// the Change list's arrows since both boxes share rows 5..9, different column since the
// Units box is narrower) increments a scroll offset (unitsOffset), up (lat2==10,lon2==6)
// decrements it, and drawCityScreen clamps it so the list never scrolls past either end.
// This test stations 7 warriors on one city's tile (more than the 5 that fit), and checks:
// clicking a row selects the right unit at offset 0, scrolling down by one shifts row 0 to
// the SECOND unit, scrolling down repeatedly clamps so the last row lands on the LAST unit
// (not further), and scrolling back up returns row 0 to the first unit again.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Tiles tiles;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define NUMWARRIORS 7

TestCase_026::TestCase_026()
{

}

TestCase_026::~TestCase_026()
{

}

int TestCase_026::number()
{
    return 26;
}

void TestCase_026::init()
{

    map.init(MAPHALFHEIGHT,MAPHALFWIDTH);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }

    // A small island surrounded by ocean.
    for (int lat=-3;lat<=3;lat++)
    {
        for (int lon=-3;lon<=3;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
        }
    }

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            for(auto &r:ALL_CORE_RESOURCES)
            {
                map.set(lat,lon).addResourceProductionRate(2);
            }
        }

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255;
    faction->green = 0;
    faction->blue = 0;
    faction->autoPlayer = false;

    factions.push_back(faction);

    // A city at the map center, with 7 warriors standing on its own tile: more than the
    // 5 that the Units box (UNITS_BOX_ROWS, cityscreenui.cpp) can show at once.
    {
        City *city = new City(&map, 0, getNextCityId(), 0, 0);
        city->setName("Kattegate");
        city->foundedyear = -4000;
        cities[city->id] = city;
        cityid = city->id;
    }

    for(int i=0;i<NUMWARRIORS;i++)
    {
        Warrior *w = new Warrior();
        w->longitude = 0;
        w->latitude = 0;
        w->id = getNextUnitId();
        w->faction = 0;
        w->availablemoves = w->getUnitMoves();

        units[w->id] = w;
    }

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

}

int TestCase_026::check(int year)
{

    ticks++;

    if (isdone)
        return 0;

    // Give the game loop a couple of ticks to settle.
    if (ticks < 3)
        return 0;

    City* city = cities[cityid];

    // The canonical stationed-units order: the SAME function drawCityScreen/
    // clickOnCityScreen use, so indices here match exactly what a click would select.
    std::vector<Unit*> stationed = getUnitsAtCity(city);

    if ((int)stationed.size() != NUMWARRIORS)
    {
        isdone = true;
        haspassed = false;
        message = std::string("getUnitsAtCity did not find all 7 warriors stationed on the city's tile.");
        return 0;
    }

    controller.view = 2;
    controller.cityid = cityid;

    coordinate c = map.to_screen(city->latitude, city->longitude);

    // Draw once so the Units box (with its up/down arrows, since 7 > UNITS_BOX_ROWS) is
    // set up and the scroll offset is confirmed to start at 0.
    drawCityScreen(c.lat, c.lon, city);

    // Row 0 (lat=5), offset 0: must select the FIRST stationed unit.
    clickOnCityScreen(5, 0, 0, 0);
    if (coordinator.a_u_id != stationed[0]->id)
    {
        isdone = true;
        haspassed = false;
        message = std::string("Clicking row 0 at offset 0 did not select the first stationed unit.");
        return 0;
    }

    // Down arrow (lat2==18, lon2==6, see cityscreenui.cpp for the derivation matching the
    // Change list's arrows): scrolls by one. Row 0 must now select the SECOND unit.
    clickOnCityScreen(0, 0, 18, 6);
    drawCityScreen(c.lat, c.lon, city);   // clamp pass, same as a real frame would do

    clickOnCityScreen(5, 0, 0, 0);
    if (coordinator.a_u_id != stationed[1]->id)
    {
        isdone = true;
        haspassed = false;
        message = std::string("Clicking the down arrow once did not scroll row 0 to the second unit.");
        return 0;
    }

    // Scroll down repeatedly (well past the point where it should clamp): with 7 units and
    // 5 rows, the offset must clamp so the LAST row (lat=9) shows the LAST unit, no further.
    for(int i=0;i<5;i++)
    {
        clickOnCityScreen(0, 0, 18, 6);
        drawCityScreen(c.lat, c.lon, city);
    }

    clickOnCityScreen(9, 0, 0, 0);
    if (coordinator.a_u_id != stationed[NUMWARRIORS-1]->id)
    {
        isdone = true;
        haspassed = false;
        message = std::string("Scrolling down past the end did not clamp the last row to the last unit.");
        return 0;
    }

    // Scroll back up repeatedly (well past the top): row 0 must return to the FIRST unit.
    for(int i=0;i<10;i++)
    {
        clickOnCityScreen(0, 0, 10, 6);
        drawCityScreen(c.lat, c.lon, city);
    }

    clickOnCityScreen(5, 0, 0, 0);

    isdone = true;
    haspassed = (coordinator.a_u_id == stationed[0]->id);
    if (!haspassed)
        message = std::string("Scrolling up past the top did not clamp row 0 back to the first unit.");

    return 0;
}
std::string TestCase_026::title()
{
    return std::string("City screen Units box scrolls (up/down arrows, same mechanism as the Change list) when there are more units than fit.");

}

bool TestCase_026::done()
{
    return isdone;
}
bool TestCase_026::passed()
{
    return haspassed;
}
std::string TestCase_026::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_026();
}
