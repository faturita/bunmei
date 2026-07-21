//  TestCase_019.cpp
//  bunmei
//
//  Created by Claude on 21/07/2026
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
#include "../diplomacy.h"
#include "../resources.h"
#include "../map.h"
#include "../coordinator.h"
#include "../units/Warrior.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"

#include "testcase_019.h"

// Diplomacy (README.md "Diplomacy and Wars"): a unit's ability to enter land owned by another
// faction depends on the DefCon status between the two factions.  landSeizure lets it in AND
// flips ownership; openBorders (without landSeizure) lets it in but the tile keeps its owner;
// neither blocks the move outright.  This test walks a Viking warrior east across three
// Roman-owned tiles, changing the Viking/Roman relationship between each step:
//   (0,1): default NO_CONTACT (landSeizure=true)         -> enters AND seizes
//   (0,2): PEACE (landSeizure=false, openBorders=false)  -> blocked, stays put
//   (0,2): TRADE_AGREEMENT (openBorders=true only)        -> enters, does NOT seize

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

TestCase_019::TestCase_019()
{

}

TestCase_019::~TestCase_019()
{

}

int TestCase_019::number()
{
    return 19;
}

void TestCase_019::init()
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

    // An island around the map center.  The default bioma (0) costs 1 move per tile.
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

    // Roman-owned tiles east of the Viking warrior: no Roman units needed, ownership alone
    // is what evaluateLandEntry() checks.
    map.set(0,1).setOwnedBy(1);
    map.set(0,2).setOwnedBy(1);

    {
        coordinate c(0,0);

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
    citynames[1] = std::queue<std::string>();

    for(int i=0;i<20;i++)
    {
        citynames[0].push("Kattegate");
        citynames[1].push("Roma");
    }

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = 1;

}

int TestCase_019::check(int year)
{

    ticks++;

    Unit *u = units[1];

    // Phase A: NO_CONTACT (default) has landSeizure=true.  Moving onto the Roman tile (0,1)
    // must enter AND seize it for the Vikings.
    if (ticks == 300)
    {
        controller.registers.pitch = 0;
        controller.registers.roll = 1;
    }

    if (ticks == 400)
    {
        if (!(u->latitude == 0 && u->longitude == 1))
        {
            isdone = true;
            haspassed = false;
            char msg[256];
            sprintf(msg,"NO_CONTACT (seizure) step: unit at (%d,%d); expected (0,1).",u->latitude,u->longitude);
            message = std::string(msg);
            return 0;
        }
        if (!map.set(0,1).isOwnedBy(0))
        {
            isdone = true;
            haspassed = false;
            message = std::string("NO_CONTACT (seizure) step: tile (0,1) was entered but not seized by faction 0.");
            return 0;
        }

        // Phase B: switch to PEACE (landSeizure=false, openBorders=false): the next tile
        // (0,2), still Roman, must now be BLOCKED outright.
        diplomacy[0][1].status = PEACE;
        diplomacy[0][1].landSeizure = false;
        diplomacy[0][1].openBorders = false;

        controller.registers.pitch = 0;
        controller.registers.roll = 1;
    }

    if (ticks == 500)
    {
        if (!(u->latitude == 0 && u->longitude == 1))
        {
            isdone = true;
            haspassed = false;
            char msg[256];
            sprintf(msg,"PEACE (blocked) step: unit at (%d,%d); expected still at (0,1) (blocked).",u->latitude,u->longitude);
            message = std::string(msg);
            return 0;
        }
        if (!map.set(0,2).isOwnedBy(1))
        {
            isdone = true;
            haspassed = false;
            message = std::string("PEACE (blocked) step: tile (0,2) changed owner even though the move should have been blocked.");
            return 0;
        }

        // Phase C: switch to TRADE_AGREEMENT (landSeizure=false, openBorders=true): the
        // Vikings may now walk onto (0,2), but it must stay Roman-owned.
        diplomacy[0][1].status = TRADE_AGREEMENT;
        diplomacy[0][1].landSeizure = false;
        diplomacy[0][1].openBorders = true;

        controller.registers.pitch = 0;
        controller.registers.roll = 1;
    }

    if (ticks == 600)
    {
        isdone = true;

        if (!(u->latitude == 0 && u->longitude == 2))
        {
            haspassed = false;
            char msg[256];
            sprintf(msg,"TRADE_AGREEMENT (open borders) step: unit at (%d,%d); expected (0,2).",u->latitude,u->longitude);
            message = std::string(msg);
            return 0;
        }

        haspassed = map.set(0,2).isOwnedBy(1) && !map.set(0,2).isOwnedBy(0);
        if (!haspassed)
            message = std::string("TRADE_AGREEMENT (open borders) step: tile (0,2) was seized even though only open borders (no landSeizure) applied.");
    }

    return 0;
}
std::string TestCase_019::title()
{
    return std::string("Diplomacy: land seizure, open borders and blocked entry per the DefCon table.");

}

bool TestCase_019::done()
{
    return isdone;
}
bool TestCase_019::passed()
{
    return haspassed;
}
std::string TestCase_019::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_019();
}
