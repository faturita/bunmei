//  TestCase_047.cpp
//  bunmei
//
//  Created by Claude on 03/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

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
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"

#include "testcase_047.h"

// @Issue: the slow slide a unit does between its departing and destination tile
// (Unit::draw() lerps oldlongitude -> longitude while `completion` climbs 0 -> 1) has no
// wrap awareness, so a one-tile step across the map's vertical (longitude) edge -- where the
// new column index lands on the far side of the map from the old one -- animates as a slide
// the "long way round", right across the whole screen.
//
// Fix (Unit::update): when |longitude - oldlongitude| is more than half the map width the
// hop wrapped the edge, so snap straight to the destination (completion = 1) -- only the
// start and end tiles show, no in-between frames. A normal interior step is unaffected and
// still animates.

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

TestCase_047::TestCase_047() {}
TestCase_047::~TestCase_047() {}

int TestCase_047::number()
{
    return 47;
}

void TestCase_047::init()
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
    faction->red = 255; faction->green = 0; faction->blue = 0;
    faction->autoPlayer = false;
    factions.push_back(faction);

    Warrior *w = new Warrior();
    w->id = getNextUnitId();
    w->faction = 0;
    w->latitude = 0;
    w->longitude = 0;
    w->availablemoves = w->getUnitMoves();
    units[w->id] = w;
    warriorid = w->id;

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = warriorid;
}

int TestCase_047::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    Unit* u = units[warriorid];
    const int rightEdge = map.maxlon - 1;
    const int leftEdge  = map.minlon;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    // 1) Step off the RIGHT edge across to the left: must snap (no slide).
    u->latitude = 0; u->longitude = rightEdge;
    u->update(0, leftEdge);
    if (!u->movementCompleted())
    {
        fail("Wrap east (right edge -> left edge) still animates instead of snapping.");
        return 0;
    }

    // 2) Step off the LEFT edge across to the right: must snap too.
    u->latitude = 0; u->longitude = leftEdge;
    u->update(0, rightEdge);
    if (!u->movementCompleted())
    {
        fail("Wrap west (left edge -> right edge) still animates instead of snapping.");
        return 0;
    }

    // 3) A normal one-tile interior step must STILL animate (completion starts < 1) and
    //    then run to completion as draw() ticks it forward.
    u->latitude = 0; u->longitude = 0;
    u->update(0, 1);
    if (u->movementCompleted())
    {
        fail("A normal interior step was wrongly snapped -- the animation is gone for every move.");
        return 0;
    }
    for (int k=0;k<12;k++) u->draw();      // draw() advances `completion` by 0.1 per call
    if (!u->movementCompleted())
    {
        fail("A normal interior step never finished animating.");
        return 0;
    }

    // 4) A one-tile step that ends ON the edge but does not cross it must animate normally.
    u->latitude = 0; u->longitude = leftEdge + 1;
    u->update(0, leftEdge);
    if (u->movementCompleted())
    {
        fail("A one-tile step onto (not across) the edge was wrongly snapped.");
        return 0;
    }

    // 5) A big latitude-only change must not trigger the longitude snap.
    u->latitude = map.maxlat - 1; u->longitude = 0;
    u->update(map.minlat, 0);
    if (u->movementCompleted())
    {
        fail("A latitude-only move was wrongly snapped by the longitude-wrap check.");
        return 0;
    }

    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_047::title()
{
    return std::string("Unit move animation snaps (no full-screen slide) when a one-tile step wraps the map's vertical edge; interior moves still animate.");
}

bool TestCase_047::done()   { return isdone; }
bool TestCase_047::passed() { return haspassed; }
std::string TestCase_047::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_047();
}
