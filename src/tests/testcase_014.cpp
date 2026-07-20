//  TestCase_014.cpp
//  bunmei
//
//  Created by Claude on 14/07/2026
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

#include "../openglutils.h"
#include "../map.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../map.h"
#include "../coordinator.h"
#include "../units/Trireme.h"
#include "../units/Warrior.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"

#include "testcase_014.h"

TestCase_014::TestCase_014()
{

}

TestCase_014::~TestCase_014()
{

}

int TestCase_014::number()
{
    return 14;
}

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

extern bool goToMode;
extern int REAL_SCREEN_WIDTH;
extern int REAL_SCREEN_HEIGHT;

static int triremeid = 0;
static int warriorid = 0;

// A left click while in GoTo mode only selects the destination: when that
// destination is one of the player's cities the click must NOT fall through
// and open the city screen (controller.view = 2), for land and naval units
// alike.  A plain click on the city (no GoTo mode) must still open it.
void TestCase_014::init()
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

    // A coastal city on the NE corner of the island (ocean at lat 4 and lon 4).
    {
        City *city = new City(&map, 0, getNextCityId(), 3, 3);
        city->setName("Kattegate");
        city->foundedyear = -4000;
        cities[city->id] = city;
    }

    // A land unit on the island.
    {
        Warrior *w = new Warrior();
        w->longitude = 1;
        w->latitude = 1;
        w->id = getNextUnitId();
        w->faction = 0;
        w->availablemoves = w->getUnitMoves();

        units[w->id] = w;
        warriorid = w->id;
    }

    // A naval unit at sea next to the city.
    {
        Trireme *t = new Trireme();
        t->longitude = 5;
        t->latitude = 3;
        t->id = getNextUnitId();
        t->faction = 0;
        t->availablemoves = t->getUnitMoves();

        units[t->id] = t;
        triremeid = t->id;
    }

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = warriorid;

}

int TestCase_014::check(int year)
{

    ticks++;

    // Give the game loop a couple of ticks to settle before simulating the clicks.
    if (ticks < 3)
        return 0;

    if (isdone)
        return 0;

    // Center the screen on the city tile: a click on the exact screen center then
    // resolves to the city coordinate (centermap keeps the center unchanged there).
    coordinate cs = map.to_screen(3,3);
    centermapinmap(cs.lat, cs.lon);

    // 1. GoTo click on the city with a LAND unit active.
    coordinator.a_u_id = warriorid;
    goToMode = true;
    processMouse(GLUT_LEFT_BUTTON, GLUT_DOWN, REAL_SCREEN_WIDTH/2, REAL_SCREEN_HEIGHT/2);

    if (controller.view == 2)
    {
        isdone = true;
        haspassed = false;
        message = "The city screen opened on a GoTo click with a land unit.";
        return 0;
    }
    if (!units[warriorid]->isAuto() || units[warriorid]->target.lat != 3 || units[warriorid]->target.lon != 3)
    {
        isdone = true;
        haspassed = false;
        message = "The GoTo click with a land unit did not set the city as target.";
        return 0;
    }

    // 2. GoTo click on the city with a NAVAL unit active.
    coordinator.a_u_id = triremeid;
    goToMode = true;
    processMouse(GLUT_LEFT_BUTTON, GLUT_DOWN, REAL_SCREEN_WIDTH/2, REAL_SCREEN_HEIGHT/2);

    if (controller.view == 2)
    {
        isdone = true;
        haspassed = false;
        message = "The city screen opened on a GoTo click with a naval unit.";
        return 0;
    }
    if (!units[triremeid]->isAuto() || units[triremeid]->target.lat != 3 || units[triremeid]->target.lon != 3)
    {
        isdone = true;
        haspassed = false;
        message = "The GoTo click with a naval unit did not set the city as target.";
        return 0;
    }

    // 3. Sanity: the same click WITHOUT GoTo mode must still open the city screen
    //    (this also proves the simulated clicks really landed on the city tile).
    processMouse(GLUT_LEFT_BUTTON, GLUT_DOWN, REAL_SCREEN_WIDTH/2, REAL_SCREEN_HEIGHT/2);

    if (controller.view != 2)
    {
        isdone = true;
        haspassed = false;
        message = "A plain click on the city no longer opens the city screen.";
        return 0;
    }

    controller.view = 1;
    isdone = true;
    haspassed = true;

    return 0;
}
std::string TestCase_014::title()
{
    return std::string("A GoTo click on a city selects the target without opening the city screen.");

}

bool TestCase_014::done()
{
    return isdone;
}
bool TestCase_014::passed()
{
    return haspassed;
}
std::string TestCase_014::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_014();
}
