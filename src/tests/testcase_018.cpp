//  TestCase_018.cpp
//  bunmei
//
//  Created by Claude on 19/07/2026
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
#include "../City.h"
#include "../Faction.h"
#include "../diplomacy.h"
#include "../resources.h"
#include "../map.h"
#include "../coordinator.h"
#include "../units/Archer.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"

#include "testcase_018.h"

// Per-faction fog of war: each faction has its OWN visibility map (mapcell::visible is a
// vector indexed by faction id).  A unit unfogs the map only for its own faction, so what
// the Vikings have explored stays fogged for the Romans and vice versa.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<std::vector<Diplomacy>> diplomacy;
extern std::vector<Resource*> resources;
extern Tiles tiles;
extern MovementCost movementcosts;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_018::TestCase_018()
{

}

TestCase_018::~TestCase_018()
{

}

int TestCase_018::number()
{
    return 18;
}

void TestCase_018::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initMovementCosts(movementcosts);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }

    // An island around the map center.
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

    Faction *faction2 = new Faction();
    faction2->id = 1;
    strcpy(faction2->name,"Romans");
    faction2->red = 0;
    faction2->green = 0;
    faction2->blue = 255;
    faction2->autoPlayer = false;

    factions.push_back(faction2);

    initDiplomacy(diplomacy, factions.size());

    // A Viking archer at (0,0) and a Roman archer at (0,6): far enough apart (unfog
    // radius is 1) so their revealed areas do not overlap.
    {
        coordinate c(0,0);

        Archer *w = new Archer();
        w->longitude = c.lon;
        w->latitude = c.lat;
        w->id = getNextUnitId();
        w->faction = 0;
        w->availablemoves = w->getUnitMoves();

        units[w->id] = w;
        map.set(c.lat,c.lon).setOwnedBy(0);
    }

    {
        coordinate c(0,6);

        Archer *w = new Archer();
        w->longitude = c.lon;
        w->latitude = c.lat;
        w->id = getNextUnitId();
        w->faction = 1;
        w->availablemoves = w->getUnitMoves();

        units[w->id] = w;
        map.set(c.lat,c.lon).setOwnedBy(1);
    }

    citynames[0] = std::queue<std::string>();
    citynames[1] = std::queue<std::string>();

    for(int i=0;i<20;i++)
    {
        citynames[0].push("Kattegate");
        citynames[0].push("Jorvik");
        citynames[1].push("Roma");
        citynames[1].push("Pompeii");
    }

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = 1;

}

int TestCase_018::check(int year)
{

    ticks++;

    // By tick 300 the map has been drawn many times: every unit has unfogged the tiles
    // around itself for ITS OWN faction only.
    if (ticks == 300)
    {
        isdone = true;
        haspassed = false;

        if (!map.set(0,0).isVisible(0) || !map.set(1,1).isVisible(0))
        {
            message = std::string("The Viking archer at (0,0) did not unfog its surroundings for faction 0.");
            return 0;
        }

        if (map.set(0,0).isVisible(1))
        {
            message = std::string("Faction 1 can see (0,0), explored only by faction 0: the visibility map is shared.");
            return 0;
        }

        if (!map.set(0,6).isVisible(1))
        {
            message = std::string("The Roman archer at (0,6) did not unfog its surroundings for faction 1.");
            return 0;
        }

        if (map.set(0,6).isVisible(0))
        {
            message = std::string("Faction 0 can see (0,6), explored only by faction 1: the visibility map is shared.");
            return 0;
        }

        if (map.set(5,5).isVisible(0) || map.set(5,5).isVisible(1))
        {
            message = std::string("The tile (5,5), never explored, is visible for some faction.");
            return 0;
        }

        haspassed = true;
    }

    return 0;
}
std::string TestCase_018::title()
{
    return std::string("Per-faction fog of war: each faction only sees its own explored map.");

}

bool TestCase_018::done()
{
    return isdone;
}
bool TestCase_018::passed()
{
    return haspassed;
}
std::string TestCase_018::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_018();
}
