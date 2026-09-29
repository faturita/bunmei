//  TestCase_078.cpp
//  bunmei
//
//  Created by faturita on 27/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../map.h"
#include "../units/Unit.h"
#include "../units/Warrior.h"
#include "../City.h"
#include "../Faction.h"
#include "../coordinator.h"
#include "../commandorder.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"

#include "testcase_078.h"

// @Issue: a unit that moves while finishing its turn does not reveal the tiles around where it
// ends up until a later year. Both ways a unit can spend its last moves on a step, through the
// real Command::MoveUnitTo:
//  (a) the step costs exactly what is left: the unit moves now and must reveal now;
//  (b) the step costs MORE than is left: moveForward() leaves the unit in place with a pending
//      move (movement debt) and endOfYear() completes it -- the unit must see around its NEW
//      tile as soon as that year ends, not one year later.
// Each unit walks away from a fully fogged world, so a visible tile two steps ahead can only
// have been revealed from the tile it arrived at.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern Factions factions;
extern Tiles tiles;
extern void endOfYear();

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_078::TestCase_078()
{

}

TestCase_078::~TestCase_078()
{

}

int TestCase_078::number()
{
    return 78;
}

void TestCase_078::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon) = mapcell(LAND);      // fogged for everybody: nobody has revealed anything

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255; faction->green = 0; faction->blue = 0;
    faction->autoPlayer = false;
    factions.push_back(faction);
    citynames[0] = std::queue<std::string>();

    auto place = [&](int lat, int lon)
    {
        Warrior *u = new Warrior();
        u->id = getNextUnitId();
        u->faction = 0;
        u->latitude = lat; u->longitude = lon;
        u->availablemoves = u->getUnitMoves();
        units[u->id] = u;
        map.set(lat,lon).setOwnedBy(0);
        return u->id;
    };

    warriorid = place(0, 0);
    debtorid  = place(10, 0);

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

}

int TestCase_078::check(int year)
{

    ticks++;

    if (isdone)
        return 0;

    if (ticks < 3)
        return 0;

    char buf[256];

    isdone = true;
    haspassed = false;

    auto step = [&](int unitid, int lat, int lon)
    {
        CommandOrder co;
        co.command = Command::MoveUnitTo;
        co.parameters.spawnid = unitid;
        co.parameters.factionid = 0;
        co.parameters.latitude = lat;
        co.parameters.longitude = lon;
        coordinator.a_u_id = unitid;
        coordinator.push(co);
        processCommandOrders();
    };

    // (a) Last move, affordable: 1 move left, a 1-cost step east.
    Unit *w = units[warriorid];
    w->availablemoves = 1;
    step(warriorid, 0, 1);
    if (w->longitude != 1 || w->availablemoves > 0)
    {
        sprintf(buf,"(a) setup: warrior at lon %d with %.2f moves, expected lon 1 and none left.", w->longitude, w->availablemoves);
        message = std::string(buf);
        return 0;
    }
    if (!map.peek(0,2).isVisible(0))
    {
        message = std::string("(a) a unit that spent its last move stepping to (0,1) did not reveal (0,2).");
        return 0;
    }

    // (b) Last move, NOT affordable: 0.5 left for a 1-cost step. The unit stays, in debt.
    Unit *d = units[debtorid];
    d->availablemoves = 0.5f;
    step(debtorid, 10, 1);
    if (d->longitude != 0 || !d->hasPendingMove())
    {
        sprintf(buf,"(b) setup: debtor at lon %d pending %d, expected it to stay at lon 0 with a pending move.", d->longitude, (int)d->hasPendingMove());
        message = std::string(buf);
        return 0;
    }

    endOfYear();                         // pays the debt (-0.5+1 >= 0) and completes the move

    if (d->longitude != 1 || d->hasPendingMove())
    {
        sprintf(buf,"(b) endOfYear did not complete the pending move: lon %d pending %d.", d->longitude, (int)d->hasPendingMove());
        message = std::string(buf);
        return 0;
    }
    if (!map.peek(10,2).isVisible(0))
    {
        message = std::string("(b) the unit arrived at (10,1) when endOfYear completed its pending move, but (10,2) is still fogged.");
        return 0;
    }

    haspassed = true;

    return 0;
}
std::string TestCase_078::title()
{
    return std::string("Fog of war: a unit spending its last moves on a step reveals around where it ends up, pending moves included.");

}

bool TestCase_078::done()
{
    return isdone;
}
bool TestCase_078::passed()
{
    return haspassed;
}
std::string TestCase_078::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_078();
}
