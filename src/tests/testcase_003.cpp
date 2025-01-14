//  TestCase_003.cpp
//  bunmei
//
//  Created by Rodrigo Ramele on 07/09/2021
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
#include "../units/Chariot.h"
#include "../units/Warrior.h"
#include "../engine.h"
#include "../tiles.h"

#include "TestCase_003.h"

TestCase_003::TestCase_003()
{

}

TestCase_003::~TestCase_003()
{

}

int TestCase_003::number()
{
    return 003;
}

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;
extern Tiles tiles;

extern int mapzoom;

extern Coordinator coordinator;

void TestCase_003::init()
{

    map.init();

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
        }


    for (int lat=-10;lat<=10;lat++)
    {
        for (int lon=-10;lon<=10;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
        }
    }

     // Pick a random number and use it to seed the land masses.
    /**int r=getRandomInteger(2,15);

    for(int i=0;i<r;i++)
    {
        int lat = getRandomInteger(map.minlat,map.maxlat-1);
        int lon = getRandomInteger(map.minlon,map.maxlon-1);

        while (getRandomInteger(0,100)>2)
        {
            map.set(lat,lon) = mapcell(LAND);
            int dir=getRandomInteger(0,3);
            if (dir==0) lat-=1;
            if (dir==1) lat+=1;
            if (dir==2) lon+=1;
            if (dir==3) lon-=1;

        }
    }

    // Fill in randomly the land masses with land.
    int energy = 100000;
    for(int rep=0;rep<5000;rep++)
    {
        int lat = getRandomInteger(map.minlat,map.maxlat-1);
        int lon = getRandomInteger(map.minlon,map.maxlon-1);

        int north,south,east,west;
        north = map(lat-1,lon).code;
        south = map(lat+1,lon).code;
        east  = map(lat,lon+1).code;
        west  = map(lat,lon-1).code;

        if (energy>0) if ((south+north+east+west)>=1)
        {
            map.set(lat,lon) = mapcell(LAND);
            energy--;
        }
    }   **/


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

    faction = new Faction();
    faction->id = 1;
    strcpy(faction->name,"Romans");
    faction->red = 255;
    faction->green = 255;
    faction->blue = 0;
    faction->autoPlayer = false;
    
    factions.push_back(faction);

    {
        coordinate c(0,0);

        Warrior *warrior = new Warrior();
        warrior->longitude = c.lon;
        warrior->latitude = c.lat;
        warrior->id = getNextUnitId();
        warrior->faction = 0;
        warrior->availablemoves = warrior->getUnitMoves();

        units[warrior->id] = warrior;
        map.set(c.lat,c.lon).setOwnedBy(0);

        c = coordinate(-3,-3);
        Chariot *chariot = new Chariot();
        chariot->longitude = c.lon;
        chariot->latitude = c.lat;
        chariot->id = getNextUnitId();
        chariot->faction = 0;
        chariot->availablemoves = chariot->getUnitMoves();

        units[chariot->id] = chariot;
        map.set(c.lat,c.lon).setOwnedBy(0);

    }

    {
        coordinate c(1,1);

        Archer *archer = new Archer();
        archer->longitude = c.lon;
        archer->latitude = c.lat;
        archer->id = getNextUnitId();
        archer->faction = 1;
        archer->availablemoves = archer->getUnitMoves();

        units[archer->id] = archer;
        map.set(c.lat,c.lon).setOwnedBy(1);

        archer = new Archer();
        archer->longitude = c.lon;
        archer->latitude = c.lat;
        archer->id = getNextUnitId();
        archer->faction = 1;
        archer->availablemoves = archer->getUnitMoves();

        units[archer->id] = archer;
        map.set(c.lat,c.lon).setOwnedBy(1);

        Settler *settler = new Settler();
        settler->longitude = c.lon;
        settler->latitude = c.lat;
        settler->id = getNextUnitId();
        settler->faction = 1;
        settler->availablemoves = settler->getUnitMoves();

        units[settler->id] = settler;
        map.set(c.lat,c.lon).setOwnedBy(1);

    }

    citynames[0] = std::queue<std::string>();       // Vikings

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

    citynames[1] = std::queue<std::string>();      

    citynames[1].push("Kattegate");
    citynames[1].push("Jorvik");
    citynames[1].push("Hedeby");
    citynames[1].push("Trondheim");
    citynames[1].push("Bergen");
    citynames[1].push("Stavanger");
    citynames[1].push("Kristiansand");
    citynames[1].push("Oslo");
    citynames[1].push("Stockholm");
    citynames[1].push("Copenhagen");
    citynames[1].push("Helsinki");
    citynames[1].push("Reykjavik");

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = 1;

}

int TestCase_003::check(int year)
{

    if (ticks==300)
    {
        units[1]->goTo(1,1);
    }

    if (ticks==900)
    {
        if (units.find(1)==units.end() || (!(units[1]->latitude==1 && units[1]->longitude==1)))
        {
            isdone=true;
            message = std::string("Attacking unit lost and did not moved into the 1,1 tile.");
            haspassed = true;
        }
        else
        {
            isdone=true;
            message = std::string("The defending unit won and the attacking unit was destroyed.");
            haspassed = false;
        }
    }


    ticks++;
    return 0;
}
std::string TestCase_003::title()
{
    return std::string("Check successfully defending unit.");

}

bool TestCase_003::done()
{
    return isdone;
}
bool TestCase_003::passed()
{
    return haspassed;
}
std::string TestCase_003::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_003();
}