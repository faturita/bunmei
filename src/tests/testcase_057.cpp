//  TestCase_057.cpp
//  bunmei
//
//  Created by Claude on 07/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cmath>
#include <vector>

#include "../map.h"
#include "../units/Unit.h"
#include "../units/Warrior.h"
#include "../units/Swordman.h"
#include "../buildings/Depot.h"
#include "../buildings/Palace.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../buildable.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../cityscreenui.h"

#include "testcase_057.h"

// @Task: CityUI, the bottom-right "Change" box where produced shields accumulate.
//  (1) getProductionStorageLayout(requiredShields): the shields grid is now sized to how
//      many shields the QUEUED buildable NEEDS (factoryRequirement(bf, SHIELDS)) and spread
//      across every row the box has -- same "fill the box" maths as getFoodStorageLayout,
//      instead of a fixed 7 px / 10-per-row grid.
//  (2) the grid starts 7 px higher (spare vertical room in the box).
//  (3) drawCityScreen draws one small icon per bf->getRequiredResources() id immediately
//      left of the queued buildable's name.
// This checks factoryRequirement + getProductionStorageLayout directly and renders
// drawCityScreen with a queued Swordman (multi-ingredient recipe) as a crash/sanity pass.

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

TestCase_057::TestCase_057() {}
TestCase_057::~TestCase_057() {}

int TestCase_057::number() { return 57; }

void TestCase_057::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initCoreResources();

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon) = mapcell(LAND);
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon).setVisible(0);

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255; faction->green = 0; faction->blue = 0;
    faction->autoPlayer = false;
    factions.push_back(faction);

    City *city = new City(&map, 0, getNextCityId(), 3, 3);
    city->setName("Kattegate");
    city->foundedyear = -4000;
    cities[city->id] = city;
    cityid = city->id;

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
}

int TestCase_057::check(int year)
{
    ticks++;
    controller.view = 2;
    controller.cityid = cityid;

    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    // ---- 1) factoryRequirement: the shield cost, straight from the recipe ----------------
    {
        WarriorFactory wf;
        if (factoryRequirement(&wf, SHIELDS) != 40)
        { fail("factoryRequirement(Warrior, SHIELDS) should be 40."); return 0; }

        SwordmanFactory sf;
        if (factoryRequirement(&sf, SHIELDS) != 40)
        { fail("factoryRequirement(Swordman, SHIELDS) should be 40."); return 0; }
        if (factoryRequirement(&sf, iron) != 25)
        { fail("factoryRequirement(Swordman, iron) should be 25 (iron branch, plenty of everything)."); return 0; }

        DepotFactory df;
        if (factoryRequirement(&df, SHIELDS) != 100 || factoryRequirement(&df, tools) != 100)
        { fail("factoryRequirement(Depot, SHIELDS/tools) should be 100/100."); return 0; }

        PalaceFactory pf;
        if (factoryRequirement(&pf, SHIELDS) != 10000)
        { fail("factoryRequirement(Palace, SHIELDS) should be 10000."); return 0; }

        // A resource the recipe never touches.
        if (factoryRequirement(&wf, iron) != 0)
        { fail("factoryRequirement(Warrior, iron) should be 0 (Warrior only needs shields)."); return 0; }
    }

    // ---- 2) getProductionStorageLayout: fills the box, never overruns it -----------------
    {
        const int boxWidth = ((9)-(4))*16;   // 80 px, must match cityscreenui.cpp
        for (int req : {1, 40, 50, 100, 200, 10000})
        {
            int ipr; float cs;
            getProductionStorageLayout(req, ipr, cs);

            if (ipr < 1)
            { fail("getProductionStorageLayout itemsPerRow < 1."); return 0; }
            if (cs <= 0.0f)
            { fail("getProductionStorageLayout colsepar <= 0."); return 0; }
            // last icon's right edge (colsepar*(ipr-1) + 7) must stay inside the box
            if (cs*(ipr-1) + 7.0f > (float)boxWidth + 0.5f)
            { fail("getProductionStorageLayout row overruns the box width."); return 0; }
        }

        // A bigger requirement packs more per row (tighter columns) than a small one.
        int iprSmall, iprBig; float csSmall, csBig;
        getProductionStorageLayout(40,  iprSmall, csSmall);
        getProductionStorageLayout(400, iprBig,   csBig);
        if (!(iprBig > iprSmall && csBig < csSmall))
        { fail("A larger shield requirement should give more icons/row at a tighter colsepar."); return 0; }
    }

    // ---- 3) drawCityScreen renders with a queued multi-ingredient buildable --------------
    {
        City* city = cities[cityid];
        coordinate c = map.to_screen(city->latitude, city->longitude);

        // nothing queued -> "Nothing", empty shield grid
        drawCityScreen(c.lat, c.lon, city);

        // queue a Swordman (needs shields + iron|copper) with a partial shield stock
        BuildableFactory* sf = new SwordmanFactory();
        while (!city->productionQueue.empty()) city->productionQueue.pop();
        city->productionQueue.push(sf);
        city->resources[SHIELDS] = 25;

        drawCityScreen(c.lat, c.lon, city);   // draws the ingredient icons + the sized grid

        // a big-cost buildable: many shield icons, tight grid, still no crash
        while (!city->productionQueue.empty()) city->productionQueue.pop();
        city->productionQueue.push(new PalaceFactory());
        city->resources[SHIELDS] = 640;
        drawCityScreen(c.lat, c.lon, city);
    }

    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_057::title()
{
    return std::string("City UI 'Change' box: getProductionStorageLayout sizes the accumulated-shields grid to the queued buildable's shield cost (factoryRequirement) and fills the box; drawCityScreen shows the recipe's ingredient icons by the buildable name.");
}

bool TestCase_057::done()   { return isdone; }
bool TestCase_057::passed() { return haspassed; }
std::string TestCase_057::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_057();
}
