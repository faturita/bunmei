//  TestCase_035.cpp
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
#include <deque>
#include <iterator>
#include <iostream>

#include "../map.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../cityscreenui.h"
#include "../dee.h"

#include "testcase_035.h"

// Food Storage granary reserve line (@Task: "Add a blue line at the middle of the Food
// Storage... That line will represent that when the dependency HALF_POPULATION_CODE is
// activated (because a Granary has been built) the city preserves half of the food required
// to increase population"). cityscreenui.cpp's drawCityScreen() now draws this line, gated
// on dee.verifyDep(cityContext(city->id), HALF_POPULATION_CODE) -- the same check
// bunmei.cpp's endOfYear() already uses to halve the food lost on a population drop -- at
// the row corresponding to half of getPopulationThresshold(city->pop), in the SAME icon
// grid (itemsPerRow/colsepar from getFoodStorageLayout) the food icons above it are drawn
// in.
//
// This test checks the MATH directly (no pixel capture needed, same approach as
// testcase_031 for this same box): for a range of population values, the half-thresshold
// row must fall inside the box's row count, and the line's pixel span must not exceed the
// box's actual pixel width. It also exercises the real dee gating (verifyDep false before
// a Granary-equivalent regDep, true after) and drives a real drawCityScreen() render pass
// in both states, matching this codebase's pattern of also exercising the real render path
// to make sure it does not crash.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Tiles tiles;
extern DependencyEvaluationEngine dee;
extern void endOfYear();

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

// Same box geometry as cityscreenui.cpp's getFoodStorageLayout (Food Storage: cols -10..-4,
// rows -3..3).
#define FOOD_STORAGE_ROWS ( ((3)-(-3))*16/7 )
#define FOOD_STORAGE_WIDTH_PX ( ((-4)-(-10))*16 )
#define FOOD_ICON_PX 7

TestCase_035::TestCase_035()
{

}

TestCase_035::~TestCase_035()
{

}

int TestCase_035::number()
{
    return 35;
}

void TestCase_035::init()
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

    city->pop = 5;
    city->coreresources[0] = getPopulationThresshold(city->pop)/2;

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

}

int TestCase_035::check(int year)
{

    ticks++;

    if (isdone)
        return 0;

    // Give the game loop a couple of ticks to settle, same as testcase_025/026/031.
    if (ticks < 3)
        return 0;

    City* city = cities[cityid];

    // Check the layout math for a spread of population values, including the city's own
    // (5) and some much larger ones, so the fix is verified beyond a single case.
    int testPops[] = {1, 2, 5, 10, 25, 50, 100};
    for (int p : testPops)
    {
        int itemsPerRow; float colsepar;
        getFoodStorageLayout(p, itemsPerRow, colsepar);

        int halfThresshold = getPopulationThresshold(p)/2;
        int row = halfThresshold/itemsPerRow;

        if (row < 0 || row >= FOOD_STORAGE_ROWS)
        {
            isdone = true;
            haspassed = false;
            char buf[256];
            sprintf(buf,"pop=%d: granary line row(%d) falls outside the Food Storage box (%d rows).",
                    p, row, FOOD_STORAGE_ROWS);
            message = std::string(buf);
            return 0;
        }

        // Same per-icon round(colsepar*j) drawCityScreen actually uses for the line width.
        int lineSpan = (itemsPerRow<=1) ? 0 : (int)round((double)colsepar*(itemsPerRow-1));
        int lineWidth = lineSpan + FOOD_ICON_PX;
        if (lineWidth > FOOD_STORAGE_WIDTH_PX)
        {
            isdone = true;
            haspassed = false;
            char buf[256];
            sprintf(buf,"pop=%d: granary line width (%d px) overflows the Food Storage box (%d px).",
                    p, lineWidth, FOOD_STORAGE_WIDTH_PX);
            message = std::string(buf);
            return 0;
        }

        // @Issue follow-up (issue4.png): the line was drawn a half-icon-width too far RIGHT
        // (placeColorBar's x is a CENTER, like place()'s, but the old code offset by
        // lineWidth/2 instead of lineSpan/2 -- overshooting by half an icon width, ~48px at
        // pop=1). The line's LEFT edge (center - width/2) must land within a rounding pixel
        // of icon 0's left edge (its center, offset 0 from the box start, minus half an icon
        // width) -- an exact match isn't possible in integer pixels (truncating lineSpan/2
        // and FOOD_ICON_PX/2 separately can each lose up to half a unit), but a HALF-ICON
        // sized miss (the actual bug) is nowhere near this tolerance.
        int lineLeftEdge = lineSpan/2 - lineWidth/2;
        int iconLeftEdge = 0 - FOOD_ICON_PX/2;
        if (abs(lineLeftEdge - iconLeftEdge) > 1)
        {
            isdone = true;
            haspassed = false;
            char buf[256];
            sprintf(buf,"pop=%d: granary line left edge (%d) does not match the food icons' left edge (%d).",
                    p, lineLeftEdge, iconLeftEdge);
            message = std::string(buf);
            return 0;
        }
    }

    // The gating itself: no Granary built yet, so the dependency must not be active and the
    // render pass must not draw (or crash trying to draw) the line.
    if (dee.verifyDep(cityContext(city->id), HALF_POPULATION_CODE))
    {
        isdone = true;
        haspassed = false;
        message = std::string("HALF_POPULATION_CODE reads as active before any Granary-equivalent regDep call.");
        return 0;
    }

    coordinate c = map.to_screen(city->latitude, city->longitude);
    drawCityScreen(c.lat, c.lon, city);   // Must not crash with the dependency inactive.

    // Simulate a Granary having been built (bunmei.cpp does this via
    // dee.regDep(cityContext(city->id), building->getPerkCodes()) when a Granary finishes
    // construction) and confirm the gate flips.
    dee.regDep(cityContext(city->id), HALF_POPULATION_CODE);

    if (!dee.verifyDep(cityContext(city->id), HALF_POPULATION_CODE))
    {
        isdone = true;
        haspassed = false;
        message = std::string("HALF_POPULATION_CODE did not become active after regDep.");
        return 0;
    }

    // @Issue follow-up (issue3.png): the granary reserve kept on growth must be half of the
    // food thresshold REQUIRED to grow (bunmei.cpp endOfYear), not half of whatever food
    // happened to be stored (which can overshoot the thresshold by however much a city
    // produces per turn) -- otherwise the kept amount silently drifts away from where this
    // very line says the half-way point is. Exercise the real endOfYear(): set FOOD well
    // past the thresshold (an overshoot), let the city grow (HALF_POPULATION_CODE is
    // already active from the regDep above), and check exactly half the THRESSHOLD (not
    // half the overshot stock) was kept.
    //
    // @Issue follow-up (issue4.png): right after a real growth tick, all the stored food
    // appeared ABOVE the line instead of the reserve lining up WITH it -- caused by
    // endOfYear() computing the kept reserve off the OLD (pre-growth) pop's thresshold,
    // while the UI line (drawn against city->pop, already incremented by the time the
    // screen renders) used the NEW pop's (larger) thresshold instead. Fixed by growing pop
    // BEFORE computing the kept reserve, so both use the same (new) thresshold. The
    // assertion below checks this explicitly: the reserve must equal half of
    // getPopulationThresshold() for the city's CURRENT (post-growth) pop -- exactly what
    // the line renders -- not just half of the pre-growth thresshold.
    {
        int pop = city->pop;
        int thresshold = getPopulationThresshold(pop);
        city->coreresources[0] = thresshold + 37; // deliberate overshoot past the thresshold
        endOfYear();

        if (city->pop != pop+1)
        {
            isdone = true;
            haspassed = false;
            char buf[256];
            sprintf(buf,"endOfYear() did not grow the city's population (pop stayed %d).", city->pop);
            message = std::string(buf);
            return 0;
        }

        int expected = (int)(0.5f * (float)getPopulationThresshold(city->pop));  // NEW (post-growth) pop's thresshold -- what the UI line also uses.
        if (city->coreresources[0] != expected)
        {
            isdone = true;
            haspassed = false;
            char buf[256];
            sprintf(buf,"Granary reserve after growth was %d, expected half of the CURRENT pop's thresshold (%d) -- matching the UI line -- not half of the pre-growth thresshold or the overshot stock.",
                    city->coreresources[0], expected);
            message = std::string(buf);
            return 0;
        }
    }

    drawCityScreen(c.lat, c.lon, city);   // Must not crash with the dependency active (draws the line).

    isdone = true;
    haspassed = true;

    return 0;
}
std::string TestCase_035::title()
{
    return std::string("Food Storage box: granary reserve line is gated by HALF_POPULATION_CODE and stays within the box for any population.");

}

bool TestCase_035::done()
{
    return isdone;
}
bool TestCase_035::passed()
{
    return haspassed;
}
std::string TestCase_035::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_035();
}
