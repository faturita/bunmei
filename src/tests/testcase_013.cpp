//  TestCase_013.cpp
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

#include "testcase_013.h"

TestCase_013::TestCase_013()
{

}

TestCase_013::~TestCase_013()
{

}

int TestCase_013::number()
{
    return 13;
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

static int triremeid = 0;
static int warrior1id = 0;
static int warrior2id = 0;

// Naval GoTo: a Trireme shipping two Warriors on a 7x7 island world.
// Phase 0: GoTo to a free coast tile sails the ship next to it and DISEMBARKS one
//          passenger on the coast (awake, no sentry); the ship stays at sea.
// Phase 1: GoTo to the own-faction coastal city sails the ship INTO the city and
//          unboards the remaining passenger with its sentry removed.
void TestCase_013::init()
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

    // A coastal city on the NE corner of the island (ocean at lat 4 and lon 4).
    {
        City *city = new City(&map, 0, getNextCityId(), 3, 3);
        city->setName("Kattegate");
        city->foundedyear = -4000;
        cities[city->id] = city;
    }

    // The Trireme at sea, SW of the island, shipping two sentried Warriors
    // (the same state moveOntoNavalUnit leaves them in when they board).
    {
        Trireme *t = new Trireme();
        t->longitude = -5;
        t->latitude = -3;
        t->id = getNextUnitId();
        t->faction = 0;
        // Enough moves to run both phases within a single turn (the testcase build
        // never ends the turn by itself: autoEndOfTurn stays false).
        t->availablemoves = 30;

        units[t->id] = t;
        triremeid = t->id;

        int wids[2];
        for (int i=0;i<2;i++)
        {
            Warrior *w = new Warrior();
            w->longitude = t->longitude;
            w->latitude = t->latitude;
            w->id = getNextUnitId();
            w->faction = 0;
            w->availablemoves = 0;
            w->sentry();

            t->board(w);
            units[w->id] = w;
            wids[i] = w->id;
        }
        warrior1id = wids[0];
        warrior2id = wids[1];

        // Phase 0: sail to the free SW coast corner and land there.
        t->goTo(-3,-3);
    }

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = triremeid;

}

int TestCase_013::check(int year)
{

    ticks++;

    if (units.find(triremeid) == units.end())
    {
        isdone = true;
        haspassed = false;
        message = "The Trireme disappeared.";
        return 0;
    }

    Trireme* t = dynamic_cast<Trireme*>(units[triremeid]);
    Unit* w1 = units.find(warrior1id) != units.end() ? units[warrior1id] : nullptr;
    Unit* w2 = units.find(warrior2id) != units.end() ? units[warrior2id] : nullptr;

    if (t == nullptr || w1 == nullptr || w2 == nullptr)
    {
        isdone = true;
        haspassed = false;
        message = "A unit disappeared during the voyage.";
        return 0;
    }

    if (phase == 0)
    {
        // One (either) of the shipped Warriors landed awake on the coast target.
        Unit* landed = nullptr;
        if (w1->latitude == -3 && w1->longitude == -3 && !w1->isSentry()) landed = w1;
        if (w2->latitude == -3 && w2->longitude == -3 && !w2->isSentry()) landed = w2;

        if (landed != nullptr && t->manifest() == 1)
        {
            if (map.peek(t->latitude,t->longitude).code != OCEAN)
            {
                isdone = true;
                haspassed = false;
                message = "The Trireme itself climbed onto the coast instead of staying at sea.";
                return 0;
            }

            printf("Phase 0 passed: a Warrior landed on the coast (-3,-3), the ship stayed at sea.\n");
            phase = 1;
            // Phase 1: sail into the own coastal city.
            t->goTo(3,3);
        }
    }
    else if (phase == 1)
    {
        // The Trireme docked on the city tile and the remaining Warrior was
        // unboarded with its sentry removed.
        if (t->latitude == 3 && t->longitude == 3)
        {
            Unit* shipped = (w1->latitude == -3 && w1->longitude == -3) ? w2 : w1;

            if (t->manifest() == 0 && shipped->latitude == 3 && shipped->longitude == 3 && !shipped->isSentry())
            {
                isdone = true;
                haspassed = true;
                return 0;
            }

            isdone = true;
            haspassed = false;
            message = "The Trireme docked but the shipped Warrior was not unboarded awake in the city.";
            return 0;
        }
    }

    if (ticks > 5000)
    {
        isdone = true;
        haspassed = false;
        if (phase == 0)
            message = "GoTo to the coast never landed a shipped Warrior.";
        else
            message = "GoTo to the city never docked the Trireme.";
    }

    return 0;
}
std::string TestCase_013::title()
{
    return std::string("Naval GoTo: a Trireme lands troops on a coast and docks into its own city.");

}

bool TestCase_013::done()
{
    return isdone;
}
bool TestCase_013::passed()
{
    return haspassed;
}
std::string TestCase_013::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_013();
}
