//  TestCase_076.cpp
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
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../cityscreenui.h"
#include "../dee.h"
#include "../units/Warrior.h"

#include "testcase_076.h"

// City headcount (@Task: "Let's modify the population model on cities"). City::pop is now
// private (getCityPop/increaseCityPop/decreaseCityPop/setCityPop) and City::getHeadCount()
// gives the people living in the city, following the table above City.cpp's
// getPopulationThresshold(): a founded city holds CITY_BASE_HEADCOUNT (500) people and never
// fewer, pop 2 is 600, pop 3 is 800, pop 5 is 1500..., and
// between two population points the headcount climbs linearly with the stored food. With a
// Granary it starts 50% up after growing, and drops below that with the food.
//
// Every expected value is derived from getPopulationThresshold() itself (base(p) = sum of the
// thressholds below p), plus the table's own numbers pinned once, so the test follows the
// thresshold if it is ever tuned but still catches a formula that drifts off the table.

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

static int base(int pop)
{
    int b = CITY_BASE_HEADCOUNT;
    for (int p=1;p<pop;p++)
        b += getPopulationThresshold(p);
    return b;
}

TestCase_076::TestCase_076()
{

}

TestCase_076::~TestCase_076()
{

}

int TestCase_076::number()
{
    return 76;
}

void TestCase_076::init()
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
    city->setName("Kattegat");
    city->foundedyear = -4000;
    cities[city->id] = city;
    cityid = city->id;

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

}

int TestCase_076::check(int year)
{

    ticks++;

    if (isdone)
        return 0;

    if (ticks < 3)
        return 0;

    City* city = cities[cityid];
    char buf[256];

    isdone = true;
    haspassed = false;

    // A founded city: pop 1, nothing stored, CITY_BASE_HEADCOUNT people (table's first line).
    if (CITY_BASE_HEADCOUNT != 500 || city->getCityPop() != 1 || city->getHeadCount() != 500)
    {
        sprintf(buf,"New city: pop %d hc %d, expected pop 1 hc 500 (CITY_BASE_HEADCOUNT=%d).", city->getCityPop(), city->getHeadCount(), CITY_BASE_HEADCOUNT);
        message = std::string(buf);
        return 0;
    }

    // The table in City.cpp, pinned: pop -> headcount with an empty food storage.
    int table[][2] = {{1,500},{2,600},{3,800},{4,1100},{5,1500},{10,5000},{20,19500},{30,44000}};
    city->resources[FOOD] = 0;
    for (auto& row : table)
    {
        city->setCityPop(row[0]);
        if (city->getHeadCount() != row[1])
        {
            sprintf(buf,"pop %d with no food: hc %d, expected %d (City.cpp table).", row[0], city->getHeadCount(), row[1]);
            message = std::string(buf);
            return 0;
        }
    }

    // Linear with food, one person per food point, and continuous across a growth: a full
    // storage at pop 5 is the same headcount as an empty one at pop 6.
    city->setCityPop(5);
    for (int f=0;f<=getPopulationThresshold(5);f++)
    {
        city->resources[FOOD] = f;
        if (city->getHeadCount() != base(5) + f)
        {
            sprintf(buf,"pop 5 food %d: hc %d, expected %d.", f, city->getHeadCount(), base(5) + f);
            message = std::string(buf);
            return 0;
        }
    }
    int full = city->getHeadCount();
    city->resources[FOOD] = 0;
    city->increaseCityPop();
    if (city->getCityPop() != 6 || city->getHeadCount() != full)
    {
        sprintf(buf,"increaseCityPop: pop %d hc %d, expected pop 6 hc %d (continuous with a full pop 5).", city->getCityPop(), city->getHeadCount(), full);
        message = std::string(buf);
        return 0;
    }
    city->decreaseCityPop();
    if (city->getCityPop() != 5)
    {
        sprintf(buf,"decreaseCityPop: pop %d, expected 5.", city->getCityPop());
        message = std::string(buf);
        return 0;
    }

    // Food outside the storage does not count: a shortage (negative, before endOfYear
    // shrinks the city) is not below the current pop, an overshoot is not above the next.
    city->resources[FOOD] = -40;
    if (city->getHeadCount() != base(5))
    {
        sprintf(buf,"negative food: hc %d, expected %d.", city->getHeadCount(), base(5));
        message = std::string(buf);
        return 0;
    }
    city->resources[FOOD] = getPopulationThresshold(5) + 37;
    if (city->getHeadCount() != base(6))
    {
        sprintf(buf,"overshot food: hc %d, expected %d.", city->getHeadCount(), base(6));
        message = std::string(buf);
        return 0;
    }

    // Granary: the real endOfYear() grows the city and keeps half the new thresshold, so the
    // headcount lands exactly 50% of the way to the next population point.
    dee.regDep(cityContext(city->id), HALF_POPULATION_CODE);
    endOfYear();
    int half = getPopulationThresshold(6)/2;
    if (city->getCityPop() != 6 || city->getHeadCount() != base(6) + half)
    {
        sprintf(buf,"after growing with a Granary: pop %d hc %d, expected pop 6 hc %d (half way up).", city->getCityPop(), city->getHeadCount(), base(6) + half);
        message = std::string(buf);
        return 0;
    }

    // ...and it falls below that 50% line with the food.
    city->resources[FOOD] = half - 25;
    if (city->getHeadCount() != base(6) + half - 25)
    {
        sprintf(buf,"food shortage with a Granary: hc %d, expected %d (below the half way line).", city->getHeadCount(), base(6) + half - 25);
        message = std::string(buf);
        return 0;
    }

    // The floor: a starving pop-1 city, and an abandoned one (endOfYear sets pop 0 before
    // deleting it), still count CITY_BASE_HEADCOUNT -- never fewer.
    city->setCityPop(1);
    city->resources[FOOD] = -80;
    int starving = city->getHeadCount();
    city->setCityPop(0);
    city->resources[FOOD] = 0;
    int abandoned = city->getHeadCount();
    city->setCityPop(6);
    city->resources[FOOD] = half;
    if (starving != CITY_BASE_HEADCOUNT || abandoned != CITY_BASE_HEADCOUNT)
    {
        sprintf(buf,"headcount floor: starving pop 1 hc %d, pop 0 hc %d, expected %d for both.", starving, abandoned, CITY_BASE_HEADCOUNT);
        message = std::string(buf);
        return 0;
    }

    // reduceHeadCount(): the people come out of the stored food; when that is not enough the
    // city drops population points (refilling the food with each point's thresshold), and a
    // pop 1 city stops at CITY_BASE_HEADCOUNT. Outside that floor, hc drops by EXACTLY amount.
    {
        struct { int pop, food, amount, expPop, expFood; } cases[] = {
            {5, 300,  100, 5, 200},     // enough food: only the food goes down
            {5,  50,  100, 4, 350},     // one point lost: 50-100+thr(4)
            {5,   0,  950, 1,  50},     // four points lost: -950+400+300+200+100
            {1,  30,  500, 1,   0},     // pop 1: floored, hc stays at the founding 500
            {2,  10,  700, 1,   0},     // drops to pop 1 and is floored there
        };
        for (auto& k : cases)
        {
            city->setCityPop(k.pop);
            city->resources[FOOD] = k.food;
            for (int lat=-3;lat<=3;lat++)
                for (int lon=-3;lon<=3;lon++)
                    city->assignTile(coordinate(lat,lon));   // fill the allowance, so a pop drop MUST release tiles
            if (city->numberOfWorkingTiles() != city->workingTileAllowance())
            {
                sprintf(buf,"setup: pop %d works %d tiles, expected its full allowance %d.", city->getCityPop(), city->numberOfWorkingTiles(), city->workingTileAllowance());
                message = std::string(buf);
                return 0;
            }
            int before = city->getHeadCount();
            city->reduceHeadCount(k.amount);
            int expHc = std::max(CITY_BASE_HEADCOUNT, before - k.amount);
            if (city->getCityPop() != k.expPop || city->resources[FOOD] != k.expFood || city->getHeadCount() != expHc)
            {
                sprintf(buf,"reduceHeadCount(%d) at pop %d food %d: got pop %d food %d hc %d, expected pop %d food %d hc %d.",
                        k.amount, k.pop, k.food, city->getCityPop(), city->resources[FOOD], city->getHeadCount(), k.expPop, k.expFood, expHc);
                message = std::string(buf);
                return 0;
            }
            if (city->numberOfWorkingTiles() > city->workingTileAllowance())
            {
                sprintf(buf,"reduceHeadCount left pop %d working %d tiles, allowance %d.", city->getCityPop(), city->numberOfWorkingTiles(), city->workingTileAllowance());
                message = std::string(buf);
                return 0;
            }
        }
    }

    // The real caller: endOfYear() builds a Warrior and takes its crew (Unit::getSize()) out
    // of the city. A pop 3 city with 50 food cannot pay 100 from food, so it drops to pop 2
    // (50 is more than a year's shortage on this map, so starvation alone would NOT drop it).
    {
        city->setCityPop(3);
        city->resources[FOOD] = 50;
        city->resources[SHIELDS] = 1000;
        while (!city->productionQueue.empty()) city->productionQueue.pop();
        city->productionQueue.push(new WarriorFactory());
        size_t unitsBefore = units.size();
        endOfYear();
        if (units.size() != unitsBefore + 1 || city->getCityPop() != 2)
        {
            sprintf(buf,"endOfYear building a Warrior at pop 3 with 50 food: units %zu->%zu, pop %d, expected one new unit and pop 2.",
                    unitsBefore, units.size(), city->getCityPop());
            message = std::string(buf);
            return 0;
        }
    }

    coordinate c = map.to_screen(city->latitude, city->longitude);
    drawCityScreen(c.lat, c.lon, city);   // Title now reads "Kattegat(Pop: hc)" -- must not crash.

    haspassed = true;

    return 0;
}
std::string TestCase_076::title()
{
    return std::string("City headcount: starts at and never drops under CITY_BASE_HEADCOUNT, follows the thresshold table, climbs linearly with food, reduceHeadCount takes people out.");

}

bool TestCase_076::done()
{
    return isdone;
}
bool TestCase_076::passed()
{
    return haspassed;
}
std::string TestCase_076::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_076();
}
