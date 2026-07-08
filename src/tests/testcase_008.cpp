//  TestCase_008.cpp
//  bunmei
//
//  Created by Claude on 08/07/2026
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
#include "../map.h"
#include "../coordinator.h"
#include "../units/Warrior.h"
#include "../engine.h"
#include "../tiles.h"

#include "testcase_008.h"

// Read the middle row of the rendered frame and measure the black margins left and right
// of the map.  Regression check for the far-plane clipping bug: quads placed at negative x
// (the west half of maps larger than 72 tiles) were silently clipped, leaving a huge black
// band on the left while the map stayed glued to the right edge.
static void measureMapMargins(int &leftmargin, int &rightmargin, int &screenwidth)
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int w = viewport[2], h = viewport[3];
    std::vector<unsigned char> row(w*4);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, h/2, w, 1, GL_RGBA, GL_UNSIGNED_BYTE, row.data());
    int first=-1, last=-1;
    for(int x=0;x<w;x++)
    {
        if (row[x*4] || row[x*4+1] || row[x*4+2])
        {
            if (first<0) first=x;
            last=x;
        }
    }
    screenwidth = w;
    leftmargin  = (first<0) ? w : first;
    rightmargin = (last<0)  ? w : w-1-last;
}

TestCase_008::TestCase_008()
{

}

TestCase_008::~TestCase_008()
{

}

int TestCase_008::number()
{
    return 8;
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

// The test runs on mapsize 2 (144x96), which covers the screen at mapzoom 0.5 (defaultzoom).
#define TEST_MAPSIZE 2

// The unit is placed far away from the map center.
#define UNIT_LAT 30
#define UNIT_LON 55

void TestCase_008::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }

    // A small island far away from the map center.
    for (int lat=UNIT_LAT-3;lat<=UNIT_LAT+3;lat++)
    {
        for (int lon=UNIT_LON-3;lon<=UNIT_LON+3;lon++)
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
                map.set(lat,lon).resource_production_rate.push_back(2);
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

    {
        coordinate c(UNIT_LAT,UNIT_LON);

        Warrior *w = new Warrior();
        w->longitude = c.lon;
        w->latitude = c.lat;
        w->id = getNextUnitId();
        w->faction = 0;
        w->availablemoves = w->getUnitMoves();

        units[w->id] = w;
        map.set(c.lat,c.lon).setOwnedBy(0);
    }

    citynames[0] = std::queue<std::string>();

    for(int i=0;i<20;i++)
    {
        citynames[0].push("Kattegate");
        citynames[0].push("Jorvik");
        citynames[0].push("Hedeby");
    }

    // Replicate the game start: initMap sets mapzoom=1 and initWorldModelling zooms in
    // to the standard game zoom (mapzoom 2), the same for every mapsize.
    mapzoom = 1;
    zoommapin();
    centermapinmap(UNIT_LAT,UNIT_LON);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = 1;

}

int TestCase_008::check(int year)
{

    ticks++;

    // The game must start at the standard zoom, independently of the mapsize.
    if (ticks == 1)
    {
        if (mapzoom != 2.0f)
        {
            isdone = true;
            haspassed = false;
            char msg[256];
            sprintf(msg,"The game did not start at the standard zoom: mapzoom is %f instead of 2.",mapzoom);
            message = std::string(msg);
            return 0;
        }
    }

    // Phase 1: at the standard zoom the camera is centered on the unit.  Zoom out once.
    if (ticks == 300)
    {
        coordinate c = getCurrentCenter();
        if (!(c.lat == UNIT_LAT && c.lon == UNIT_LON))
        {
            isdone = true;
            haspassed = false;
            char msg[256];
            sprintf(msg,"Standard zoom: center is (%d,%d) instead of the unit (%d,%d).",c.lat,c.lon,UNIT_LAT,UNIT_LON);
            message = std::string(msg);
            return 0;
        }
        zoommapout();
    }

    // Phase 2: one zoom out (mapzoom 1): the map still does not fit, the camera stays on the unit.  Zoom out again.
    if (ticks == 600)
    {
        coordinate c = getCurrentCenter();
        if (mapzoom != 1.0f || !(c.lat == UNIT_LAT && c.lon == UNIT_LON))
        {
            isdone = true;
            haspassed = false;
            char msg[256];
            sprintf(msg,"First zoom out: mapzoom %f, center (%d,%d); expected mapzoom 1 centered on the unit (%d,%d).",mapzoom,c.lat,c.lon,UNIT_LAT,UNIT_LON);
            message = std::string(msg);
            return 0;
        }
        zoommapout();
    }

    // Phase 3: two zoom outs reach the defaultzoom: the whole map covers the screen and the camera locks centered.
    if (ticks == 900)
    {
        coordinate c = getCurrentCenter();
        if (mapzoom != getMapDimension(TEST_MAPSIZE).defaultzoom || !(c.lat == 0 && c.lon == 0))
        {
            isdone = true;
            haspassed = false;
            char msg[256];
            sprintf(msg,"Second zoom out: mapzoom %f, center (%d,%d); expected mapzoom %f centered on the map (0,0).",mapzoom,c.lat,c.lon,getMapDimension(TEST_MAPSIZE).defaultzoom);
            message = std::string(msg);
            return 0;
        }

        // The RENDERED map must really cover the screen: small and similar black margins on
        // both sides (the far-plane clipping bug blacked out the whole west half of the map).
        int leftmargin, rightmargin, screenwidth;
        measureMapMargins(leftmargin, rightmargin, screenwidth);
        if (leftmargin > screenwidth/20 || rightmargin > screenwidth/20)
        {
            isdone = true;
            haspassed = false;
            char msg[256];
            sprintf(msg,"Full map view does not cover the screen: black margins left=%dpx right=%dpx of %dpx.",leftmargin,rightmargin,screenwidth);
            message = std::string(msg);
            return 0;
        }

        // Try to zoom out beyond the full map view: it must be clamped.
        zoommapout();
    }

    // Phase 4: zooming out further is clamped and the camera stays locked to the map center.
    if (ticks == 1200)
    {
        coordinate c = getCurrentCenter();
        isdone = true;
        haspassed = (mapzoom == getMapDimension(TEST_MAPSIZE).defaultzoom && c.lat == 0 && c.lon == 0);
        if (!haspassed)
        {
            char msg[256];
            sprintf(msg,"Zoom out beyond the full map: mapzoom %f, center (%d,%d); expected clamped at %f and centered (0,0).",mapzoom,c.lat,c.lon,getMapDimension(TEST_MAPSIZE).defaultzoom);
            message = std::string(msg);
        }
    }

    return 0;
}
std::string TestCase_008::title()
{
    return std::string("The game starts at the standard zoom and zooming out N times centers the full mapsize N map.");

}

bool TestCase_008::done()
{
    return isdone;
}
bool TestCase_008::passed()
{
    return haspassed;
}
std::string TestCase_008::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_008();
}
