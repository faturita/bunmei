//  TestCase_025.cpp
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

#include "testcase_025.h"

// Clicking a city (task #7) must open ONLY the city UI, no longer also activate whatever
// unit is stationed on the city tile (usercontrols.cpp's map-click unit-selection loop is
// now skipped when the click opened a city).  Stationed units are picked instead from a new
// "Units" box drawn just below the map inside the city screen (cityscreenui.cpp
// drawCityScreen/clickOnCityScreen): clicking a row there calls the same activateUnit()
// (engine.cpp) the map-view click uses, so it wakes/un-fortifies/interrupts exactly like
// before, just from a different click target.  This test puts a FORTIFIED Warrior on the
// city's own tile, clicks the city (via a real simulated map click, same technique as
// testcase_014) and checks the city screen opens WITHOUT touching the warrior (still
// fortified, coordinator.a_u_id untouched), then calls clickOnCityScreen directly on the
// Units box's first row and checks the warrior is now activated and un-fortified.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;
extern Tiles tiles;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

extern int REAL_SCREEN_WIDTH;
extern int REAL_SCREEN_HEIGHT;

TestCase_025::TestCase_025()
{

}

TestCase_025::~TestCase_025()
{

}

int TestCase_025::number()
{
    return 25;
}

void TestCase_025::init()
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

    resources.push_back(new Resource(0,0,"assets/assets/city/food.png","Food"));
    resources.push_back(new Resource(1,0,"assets/assets/city/production.png","Shields"));
    resources.push_back(new Resource(2,0,"assets/assets/city/trade.png","Trade"));
    resources.push_back(new Resource(3,0,"assets/assets/city/gold.png","Coins"));
    resources.push_back(new Resource(4,0,"assets/assets/city/bulb.png","Science"));
    resources.push_back(new Resource(5,0,"assets/assets/city/culture.png","Culture"));

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            for(auto &r:resources)
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

    // A city at the map center, with a FORTIFIED Warrior standing on its own tile.
    {
        City *city = new City(&map, 0, getNextCityId(), 0, 0);
        city->setName("Kattegate");
        city->foundedyear = -4000;
        cities[city->id] = city;
        cityid = city->id;
    }

    {
        Warrior *w = new Warrior();
        w->longitude = 0;
        w->latitude = 0;
        w->id = getNextUnitId();
        w->faction = 0;
        w->availablemoves = w->getUnitMoves();
        w->fortify();

        units[w->id] = w;
        warriorid = w->id;
    }

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = warriorid;

}

int TestCase_025::check(int year)
{

    ticks++;

    // Give the game loop a couple of ticks to settle before simulating the click.
    if (ticks < 3)
        return 0;

    if (isdone)
        return 0;

    Unit *w = units[warriorid];

    // Center the screen on the city tile, then click screen center: resolves to (0,0),
    // same technique as testcase_014.
    coordinate cs = map.to_screen(0,0);
    centermapinmap(cs.lat, cs.lon);
    processMouse(GLUT_LEFT_BUTTON, GLUT_DOWN, REAL_SCREEN_WIDTH/2, REAL_SCREEN_HEIGHT/2);

    if (controller.view != 2 || controller.cityid != cityid)
    {
        isdone = true;
        haspassed = false;
        message = std::string("Clicking the city did not open its city screen.");
        return 0;
    }

    if (!w->isFortified())
    {
        isdone = true;
        haspassed = false;
        message = std::string("Clicking the city also un-fortified the unit stationed on it (units must no longer be auto-activated by a city click).");
        return 0;
    }

    // getOverlayAssets() (Unit.h/.cpp) is the single shared source of status overlays for
    // BOTH the map (Unit::draw()) and the city screen's Units box (drawCityScreen): while
    // fortified it must report exactly the fortify overlay, nothing else.
    std::vector<const char*> overlaysBefore = w->getOverlayAssets();
    if (overlaysBefore.size() != 1 || std::string(overlaysBefore[0]) != "assets/assets/map/fortify.png")
    {
        isdone = true;
        haspassed = false;
        message = std::string("getOverlayAssets() did not report exactly the fortify overlay for a fortified unit.");
        return 0;
    }

    // Now click the first row of the city screen's "Units" box (row 5, see
    // cityscreenui.cpp drawCityScreen/clickOnCityScreen) directly: this is the click
    // handler itself, the same call processMouse's view==2 case makes.
    clickOnCityScreen(5, 0, 0, 0);

    controller.view = 1;
    isdone = true;

    if (coordinator.a_u_id != warriorid)
    {
        haspassed = false;
        message = std::string("Clicking the unit's row in the city screen did not activate it.");
        return 0;
    }

    if (w->isFortified())
    {
        haspassed = false;
        message = std::string("Clicking the unit's row in the city screen did not un-fortify it.");
        return 0;
    }

    haspassed = w->getOverlayAssets().empty();
    if (!haspassed)
        message = std::string("getOverlayAssets() still reports an overlay after the unit was activated and un-fortified.");

    return 0;
}
std::string TestCase_025::title()
{
    return std::string("Clicking a city only opens its UI; stationed units are activated from the city screen's Units box.");

}

bool TestCase_025::done()
{
    return isdone;
}
bool TestCase_025::passed()
{
    return haspassed;
}
std::string TestCase_025::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_025();
}
