//  TestCase_036.cpp
//  bunmei
//
//  Created by Claude on 24/08/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <map>
#include <iterator>

#include "../map.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../commandorder.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../dee.h"
#include "../buildings/Granary.h"

#include "testcase_036.h"

// @Task: Buildings are built only once per city -- populateCityBuildables() (engine.cpp)
// must exclude a Buildable*Factory from city->buildable once a Building of the same kind
// already sits in city->buildings, while leaving UNIT factories (which have no such limit,
// e.g. a city can keep building Warriors/Settlers) untouched. Factory and Building instance
// share the same name string (e.g. GranaryFactory/Granary are both "Granary"), so
// populateCityBuildables() now skips a factory if any entry in city->buildings has a
// matching name.
//
// This test: (1) registers TECH_POTTERY so GranaryFactory clears its dependency gate and
// confirms "Granary" appears in city->buildable; (2) adds a Granary directly to
// city->buildings (simulating it having finished construction) and re-populates, checking
// "Granary" is now ABSENT while other ungated entries (units, and another building,
// "Barracks") are still present -- so the fix doesn't over-exclude.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;
extern Tiles tiles;
extern DependencyEvaluationEngine dee;

extern float mapzoom;

extern Coordinator coordinator;

#define TEST_MAPSIZE 1

TestCase_036::TestCase_036()
{

}

TestCase_036::~TestCase_036()
{

}

int TestCase_036::number()
{
    return 36;
}

void TestCase_036::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }

    for (int lat=-8;lat<=8;lat++)
        for (int lon=-8;lon<=8;lon++)
            map.set(lat,lon) = mapcell(LAND);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon).setVisible(0);

    resources.push_back(new Resource(0,0,"assets/assets/city/food.png","Food"));
    resources.push_back(new Resource(1,0,"assets/assets/city/production.png","Shields"));
    resources.push_back(new Resource(2,0,"assets/assets/city/trade.png","Trade"));
    resources.push_back(new Resource(3,0,"assets/assets/city/gold.png","Coins"));
    resources.push_back(new Resource(4,0,"assets/assets/city/bulb.png","Science"));
    resources.push_back(new Resource(5,0,"assets/assets/city/culture.png","Culture"));

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255;
    faction->green = 0;
    faction->blue = 0;
    faction->autoPlayer = false;

    factions.push_back(faction);

    City *city = new City(&map, 0, getNextCityId(), 0, 0);
    city->setName("Kattegate");
    city->foundedyear = -4000;
    cities[city->id] = city;
    cityid = city->id;

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

    // Clear the Granary/Barracks/etc. tech gates so they show up in buildable regardless
    // of the (not-yet-implemented) science mechanism -- this test is about the ALREADY
    // BUILT exclusion, not dependency gating.
    dee.regDep(factionContext(0), TECH_POTTERY);
    dee.regDep(factionContext(0), TECH_WARRIOR_CODE);

}

// Pushes Command::PopulateBuildableOrder for the test city and drains it, same as opening the
// city UI's Change list does (cityscreenui.cpp clickOnCityScreen).
static void openChangeList(int cityid)
{
    CommandOrder co;
    co.command = Command::PopulateBuildableOrder;
    co.parameters.cityid = cityid;
    coordinator.push(co);
    processCommandOrders();
}

static bool hasFactoryNamed(City* city, const char* name)
{
    for (auto& bf : city->buildable)
        if (strcmp(bf->name, name) == 0)
            return true;
    return false;
}

int TestCase_036::check(int year)
{

    ticks++;

    if (isdone)
        return 0;

    // Give the game loop a couple of ticks to settle, same as testcase_025/026/029/033.
    if (ticks < 3)
        return 0;

    City* city = cities[cityid];

    openChangeList(cityid);

    if (!hasFactoryNamed(city, "Granary"))
    {
        isdone = true;
        haspassed = false;
        message = std::string("Granary should be buildable before any Granary has been built in this city.");
        return 0;
    }
    if (!hasFactoryNamed(city, "Barracks"))
    {
        isdone = true;
        haspassed = false;
        message = std::string("Barracks should be buildable before any Barracks has been built in this city.");
        return 0;
    }
    if (!hasFactoryNamed(city, "Warrior"))
    {
        isdone = true;
        haspassed = false;
        message = std::string("Warrior (a unit, no once-per-city limit) should be buildable.");
        return 0;
    }

    // Simulate a Granary having finished construction (bunmei.cpp's endOfYear() does this
    // via c->buildings.push_back(building) when the production queue completes it).
    city->buildings.push_back(new Granary());

    openChangeList(cityid);

    if (hasFactoryNamed(city, "Granary"))
    {
        isdone = true;
        haspassed = false;
        message = std::string("Granary is still buildable after this city already built one -- buildings must be built only once per city.");
        return 0;
    }
    if (!hasFactoryNamed(city, "Barracks"))
    {
        isdone = true;
        haspassed = false;
        message = std::string("Barracks was incorrectly excluded after only a Granary (a DIFFERENT building) was built.");
        return 0;
    }
    if (!hasFactoryNamed(city, "Warrior"))
    {
        isdone = true;
        haspassed = false;
        message = std::string("Warrior (a unit) was incorrectly excluded after a building was built -- the once-per-city limit must not apply to units.");
        return 0;
    }

    isdone = true;
    haspassed = true;

    return 0;
}
std::string TestCase_036::title()
{
    return std::string("A city's buildable list excludes a Building's factory once that Building has already been built there, but never excludes Units.");

}

bool TestCase_036::done()
{
    return isdone;
}
bool TestCase_036::passed()
{
    return haspassed;
}
std::string TestCase_036::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_036();
}
