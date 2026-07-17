//  TestCase_015.cpp
//  bunmei
//
//  Created by Claude on 15/07/2026
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

#include "testcase_015.h"

// Roads are drawn by COMPOSITING one half-segment sprite per neighbouring road tile
// (map.cpp).  Regression check for the single-sprite lookup bug: a straight W-E road
// produced direction mask 0x05, which had no tiles[] entry, so NOTHING was drawn on the
// middle tile; diagonal road neighbours were never checked at all.
// The test reads back the rendered pixels of the map center tile and requires them to
// change when roads are built around it.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;
extern Tiles tiles;

extern float mapzoom;
extern int width;
extern int height;

extern Coordinator coordinator;

#define TEST_MAPSIZE 1

// The active (blinking) unit is parked here, away from the sampled center tile.
#define UNIT_LAT 5
#define UNIT_LON 5

// Read back the rendered center tile of the screen (the map is centered on tile 0,0):
// a rectangle of tilew x tileh device pixels around the screen center, RGB only.
static void captureCenterTile(std::vector<unsigned char> &pixels, int &tilew, int &tileh)
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int w = viewport[2], h = viewport[3];

    // The glOrtho window spans width/mapzoom logical pixels over w device pixels,
    // and a tile is 16 logical pixels.
    tilew = (int)(16.0f * w * mapzoom / (float)width);
    tileh = (int)(16.0f * h * mapzoom / (float)height);

    pixels.resize(tilew*tileh*4);
    glReadBuffer(GL_FRONT);
    glReadPixels(w/2 - tilew/2, h/2 - tileh/2, tilew, tileh, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
}

// Count the pixels that differ between two captures inside the given column range.
static int changedPixels(const std::vector<unsigned char> &a, const std::vector<unsigned char> &b,
                         int tilew, int tileh, int colmin, int colmax)
{
    int changed = 0;
    for(int y=0;y<tileh;y++)
        for(int x=colmin;x<colmax;x++)
        {
            int i = (y*tilew+x)*4;
            if (abs(a[i]-b[i])>8 || abs(a[i+1]-b[i+1])>8 || abs(a[i+2]-b[i+2])>8)
                changed++;
        }
    return changed;
}

TestCase_015::TestCase_015()
{

}

TestCase_015::~TestCase_015()
{

}

int TestCase_015::number()
{
    return 15;
}

void TestCase_015::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }

    // An island around the map center, where the roads will be built.
    for (int lat=-8;lat<=8;lat++)
    {
        for (int lon=-8;lon<=8;lon++)
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

    // Replicate the game start zoom and center the camera on the tile under test.
    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = 1;

}

int TestCase_015::check(int year)
{

    ticks++;

    int tilew, tileh;

    // Phase 1: capture the center tile without any road, then build a straight
    // west-east road through it: roads on (0,-1), (0,0) and (0,1).
    if (ticks == 300)
    {
        captureCenterTile(baseline, tilew, tileh);

        map.set(0,-1).buildRoad();
        map.set(0, 0).buildRoad();
        map.set(0, 1).buildRoad();
    }

    // Phase 2: the middle tile of a straight road must show a road segment on BOTH
    // halves of the tile (towards the west and east neighbours).  Then add a road on
    // the NE diagonal neighbour of the center tile.
    if (ticks == 600)
    {
        std::vector<unsigned char> now;
        captureCenterTile(now, tilew, tileh);

        int left  = changedPixels(baseline, now, tilew, tileh, 0, tilew/2);
        int right = changedPixels(baseline, now, tilew, tileh, tilew/2, tilew);
        if (left == 0 || right == 0)
        {
            isdone = true;
            haspassed = false;
            char msg[256];
            sprintf(msg,"Straight W-E road: the center tile did not change on both halves (left %d px, right %d px changed).",left,right);
            message = std::string(msg);
            return 0;
        }

        straightroad = now;
        map.set(-1,1).buildRoad();
    }

    // Phase 3: the diagonal neighbour must add a new segment on the center tile.
    if (ticks == 900)
    {
        std::vector<unsigned char> now;
        captureCenterTile(now, tilew, tileh);

        int changed = changedPixels(straightroad, now, tilew, tileh, 0, tilew);
        isdone = true;
        haspassed = (changed > 0);
        if (!haspassed)
        {
            char msg[256];
            sprintf(msg,"Diagonal NE road neighbour: the center tile did not change (%d px changed).",changed);
            message = std::string(msg);
        }
    }

    return 0;
}
std::string TestCase_015::title()
{
    return std::string("Road tiles composite one segment per neighbouring road, diagonals included.");

}

bool TestCase_015::done()
{
    return isdone;
}
bool TestCase_015::passed()
{
    return haspassed;
}
std::string TestCase_015::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_015();
}
