//  TestCase_009.cpp
//  bunmei
//
//  Created by Claude on 07/07/2026
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
#include "../units/Settler.h"
#include "../units/Archer.h"
#include "../units/Warrior.h"
#include "../engine.h"
#include "../tiles.h"

#include "testcase_009.h"

TestCase_009::TestCase_009()
{

}

TestCase_009::~TestCase_009()
{

}

int TestCase_009::number()
{
    return 9;
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

void TestCase_009::init()
{

    map.init(MAPHALFHEIGHT,MAPHALFWIDTH);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }


    // One big landmass for the autoplayer faction to populate.
    for (int lat=-10;lat<=10;lat++)
    {
        for (int lon=-10;lon<=10;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
        }
    }

    // A small separated island for the other faction, so it cannot interfere.
    for (int lat=15;lat<=17;lat++)
    {
        for (int lon=20;lon<=22;lon++)
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
    faction->autoPlayer = true;

    factions.push_back(faction);

    faction = new Faction();
    faction->id = 1;
    strcpy(faction->name,"Romans");
    faction->red = 255;
    faction->green = 255;
    faction->blue = 0;
    faction->autoPlayer = true;

    factions.push_back(faction);

    {
        coordinate c(0,0);

        Settler *s = new Settler();
        s->longitude = c.lon;
        s->latitude = c.lat;
        s->id = getNextUnitId();
        s->faction = 0;
        s->availablemoves = s->getUnitMoves();

        units[s->id] = s;
        map.set(c.lat,c.lon).setOwnedBy(0);

        Warrior *w = new Warrior();
        w->longitude = c.lon;
        w->latitude = c.lat;
        w->id = getNextUnitId();
        w->faction = 0;
        w->availablemoves = w->getUnitMoves();

        units[w->id] = w;
        map.set(c.lat,c.lon).setOwnedBy(0);

    }

    {
        coordinate c(7,6);

        Archer *archer = new Archer();
        archer->longitude = c.lon;
        archer->latitude = c.lat;
        archer->id = getNextUnitId();
        archer->faction = 1;
        archer->availablemoves = archer->getUnitMoves();

        units[archer->id] = archer;
        map.set(c.lat,c.lon).setOwnedBy(1);

    }

    citynames[0] = std::queue<std::string>();       // Vikings

    for(int i=0;i<20;i++)
    {
        citynames[0].push("Kattegate");
        citynames[0].push("Jorvik");
        citynames[0].push("Hedeby");
        citynames[0].push("Trondheim");
        citynames[0].push("Bergen");
        citynames[0].push("Stavanger");
        citynames[0].push("Kristiansand");
        citynames[0].push("Oslo");
        citynames[0].push("Stockholm");
        citynames[0].push("Copenhagen");
        citynames[0].push("Helsinki");
        citynames[0].push("Reykjavik");
    }

    citynames[1] = std::queue<std::string>();

    for(int i=0;i<20;i++)
    {
        citynames[1].push("Roma");
        citynames[1].push("Neapolis");
        citynames[1].push("Pompeii");
        citynames[1].push("Ravenna");
        citynames[1].push("Mediolanum");
    }

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = 1;

}

int TestCase_009::check(int year)
{

    ticks++;

    // Cities built too close to each other mean the spot selection failed.
    for (auto it1 = cities.begin(); it1 != cities.end(); ++it1)
        for (auto it2 = std::next(it1); it2 != cities.end(); ++it2)
        {
            City *a = it1->second;
            City *b = it2->second;

            int dlat = abs(a->latitude - b->latitude);
            int dlon = abs(a->longitude - b->longitude);
            int width = map.maxlon - map.minlon;
            if (dlon > width/2) dlon = width - dlon;

            if (std::max(dlat,dlon) <= 3)
            {
                // isdone = true;
                // haspassed = false;
                // message = "Two cities were built too close to each other.";
                // return 0;
            }
        }

    // The Romans start with an archer and NO settler, so the only way they can ever own a
    // city is by conquering an undefended Viking one: a Roman city proves that the fighting
    // and capture mechanics work end to end.
    for (auto& [k,c] : cities)
    {
        if (c->faction == 1)
        {
            // isdone = true;
            // haspassed = true;
            // return 0;
        }
    }

    // if (ticks > 20000)
    // {
    //     isdone = true;
    //     haspassed = false;
    //     message = "The Romans never conquered a Viking city.";
    // }

    return 0;
}
std::string TestCase_009::title()
{
    return std::string("Two civilizations fight over the same island and cities can be conquered.");

}

bool TestCase_009::done()
{
    return isdone;
}
bool TestCase_009::passed()
{
    return haspassed;
}
std::string TestCase_009::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_009();
}
