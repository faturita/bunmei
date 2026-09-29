//  TestCase_084.cpp
//  bunmei
//
//  Created by faturita on 29/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "../map.h"
#include "../units/Unit.h"
#include "../units/Settler.h"
#include "../units/Warrior.h"
#include "../units/Swordman.h"
#include "../City.h"
#include "../Faction.h"
#include "../buildable.h"
#include "../coordinator.h"
#include "../commandorder.h"
#include "../engine.h"
#include "../automation.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../diplomacy.h"
#include "../dee.h"
#include "../technologies.h"

#include "testcase_084.h"

// The AI changes the model only through CommandOrders, so a match log holds everything it did:
//   * autoPlayerCities() picks production with PopulateBuildableOrder + ChangeProductionOrder,
//     and only among what the faction can build (the rule ChangeProductionOrder enforces);
//   * a Settler walks to a city spot, and a Swordman to the nearest enemy, through
//     SetUnitDestinationOrder -- and a unit gets ONE order per decision (the destination is only
//     set when the queue is processed, so the AI must not also add a random move on top).
// The AI functions are called directly on human factions, so the tester's own game loop does
// not run the AI in between.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern Factions factions;
extern Tiles tiles;
extern MovementCost movementcosts;
extern DependencyEvaluationEngine dee;
extern TechTree techtree;
extern DiplomacyTable diplomacy;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_084::TestCase_084()
{

}

TestCase_084::~TestCase_084()
{

}

int TestCase_084::number()
{
    return 84;
}

static std::vector<CommandOrder> drain()
{
    std::vector<CommandOrder> out;
    while (!coordinator.empty())
        out.push_back(coordinator.pop());
    return out;
}

static void requeue(const std::vector<CommandOrder>& orders)
{
    for (auto& co : orders)
        coordinator.push(co);
}

void TestCase_084::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initMovementCosts(movementcosts);
    initNaming(citynames);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(OCEAN);
            map.set(lat,lon).setVisible(0);
        }

    auto land = [&](int lat0, int lat1, int lon0, int lon1)
    {
        for (int lat=lat0;lat<=lat1;lat++)
            for (int lon=lon0;lon<=lon1;lon++)
            {
                map.set(lat,lon) = mapcell(LAND);
                map.set(lat,lon).bioma = GRASSLAND;
                map.set(lat,lon).setVisible(0);
            }
    };
    land(-1, 1, -21, -19);      // a 3x3 island: no room for another city, so the AI builds military
    land(2, 20, 2, 30);         // a continent: city spots, a settler, a swordman and an enemy

    for (int row : { 0, 1 })
    {
        Faction *f = createFaction(row);
        f->autoPlayer = false;          // the AI is driven by hand below, never by the game loop
        factions.push_back(f);
    }
    initDiplomacy(diplomacy, (int)factions.size());
    initTechnologies(techtree, (int)factions.size(), dee);

    City *city = new City(&map, 0, getNextCityId(), 0, -20);
    city->setName("Kattegate");
    city->setCityPop(2);
    city->foundedyear = -4000;
    cities[city->id] = city;
    cityid = city->id;

    // The settler stands on a tile a city already claims: not a spot to build on, so it walks.
    Settler *s = new Settler();
    s->id = getNextUnitId(); s->faction = 0;
    s->latitude = 10; s->longitude = 10;
    s->availablemoves = s->getUnitMoves();
    units[s->id] = s;
    map.set(10,10).setCityOwnership(0, 999);
    settlerid = s->id;

    Swordman *w = new Swordman();
    w->id = getNextUnitId(); w->faction = 0;
    w->latitude = 5; w->longitude = 5;
    w->availablemoves = w->getUnitMoves();
    units[w->id] = w;
    swordmanid = w->id;

    Warrior *enemy = new Warrior();
    enemy->id = getNextUnitId(); enemy->faction = 1;
    enemy->latitude = 5; enemy->longitude = 12;
    enemy->availablemoves = enemy->getUnitMoves();
    units[enemy->id] = enemy;

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

}

int TestCase_084::check(int year)
{

    ticks++;

    if (isdone)
        return 0;

    if (ticks < 3)
        return 0;

    isdone = true;
    haspassed = false;
    char buf[300];

    coordinator.a_f_id = 0;
    drain();
    City *city = cities[cityid];

    // ---- production -----------------------------------------------------------------------
    // Repeats autoPlayerCities() on an idle city and returns the ids it chose (0 = failure,
    // with `message` set).
    auto produce = [&](int times, std::vector<int>& chosen) -> bool
    {
        for (int i = 0; i < times; i++)
        {
            while (!city->productionQueue.empty()) city->productionQueue.pop();
            autoPlayerCities();

            if (!city->productionQueue.empty())
            {
                message = std::string("autoPlayerCities() changed the production queue itself instead of pushing commands.");
                return false;
            }
            std::vector<CommandOrder> orders = drain();
            if (orders.size() != 2 || orders[0].command != Command::PopulateBuildableOrder ||
                orders[1].command != Command::ChangeProductionOrder ||
                orders[0].parameters.cityid != cityid || orders[1].parameters.cityid != cityid)
            {
                snprintf(buf, sizeof(buf), "autoPlayerCities() pushed %zu commands, expected PopulateBuildableOrder + ChangeProductionOrder for the city.", orders.size());
                message = std::string(buf);
                return false;
            }
            requeue(orders);
            processCommandOrders();

            int id = orders[1].parameters.selectedbuildableid;
            if (city->productionQueue.empty() || city->productionQueue.front()->getId() != id)
            {
                snprintf(buf, sizeof(buf), "ChangeProductionOrder for buildable %d was refused: the AI chose something the city cannot build.", id);
                message = std::string(buf);
                return false;
            }
            chosen.push_back(id);
        }
        return true;
    };

    std::vector<int> chosen;
    if (!produce(40, chosen)) return 0;
    for (int id : chosen)
        if (id == BUILDABLE_SWORDMAN || id == BUILDABLE_SETTLER)
        {
            snprintf(buf, sizeof(buf), "the AI chose buildable %d on an island with no room, without the technology.", id);
            message = std::string(buf);
            return 0;
        }

    // Grant the Swordman's technologies: now it is among the choices.
    BuildableFactory *swordman = buildableFactoryById(BUILDABLE_SWORDMAN);
    for (int code : swordman->getDependencyCodes())
        dee.regDep(factionContext(0), code);
    chosen.clear();
    if (!produce(40, chosen)) return 0;
    bool sword = false;
    for (int id : chosen)
        if (id == BUILDABLE_SWORDMAN)
            sword = true;
    if (!sword)
    {
        message = std::string("with its technologies known, the AI never chose a Swordman in 40 tries.");
        return 0;
    }

    // ---- the settler walks to a spot ------------------------------------------------------
    coordinator.a_u_id = settlerid;
    autoPlayerMoveUnits();
    std::vector<CommandOrder> orders = drain();
    if (units[settlerid]->isAuto())
    {
        message = std::string("the settler's destination was set directly, not through a command.");
        return 0;
    }
    if (orders.size() != 1 || orders[0].command != Command::SetUnitDestinationOrder ||
        orders[0].parameters.spawnid != settlerid || orders[0].parameters.factionid != 0)
    {
        snprintf(buf, sizeof(buf), "the settler got %zu commands, expected one SetUnitDestinationOrder.", orders.size());
        message = std::string(buf);
        return 0;
    }
    coordinate spot(orders[0].parameters.latitude, orders[0].parameters.longitude);
    requeue(orders);
    processCommandOrders();
    if (!units[settlerid]->isAuto() || !(units[settlerid]->target == spot))
    {
        message = std::string("the settler's SetUnitDestinationOrder did not give it that destination.");
        return 0;
    }

    // ---- the swordman goes for the enemy, and only that --------------------------------------
    coordinator.a_u_id = swordmanid;
    autoPlayerMoveUnits();
    orders = drain();
    if (units[swordmanid]->isAuto())
    {
        message = std::string("the swordman's destination was set directly, not through a command.");
        return 0;
    }
    if (orders.size() != 1 || orders[0].command != Command::SetUnitDestinationOrder ||
        orders[0].parameters.spawnid != swordmanid ||
        orders[0].parameters.latitude != 5 || orders[0].parameters.longitude != 12)
    {
        snprintf(buf, sizeof(buf), "the swordman got %zu commands (first %d), expected ONE SetUnitDestinationOrder to the enemy at (5,12).",
                 orders.size(), orders.empty() ? -1 : (int)orders[0].command);
        message = std::string(buf);
        return 0;
    }
    requeue(orders);
    processCommandOrders();
    if (!units[swordmanid]->isAuto() || !(units[swordmanid]->target == coordinate(5,12)))
    {
        message = std::string("the swordman's SetUnitDestinationOrder did not give it the enemy as destination.");
        return 0;
    }

    haspassed = true;

    return 0;
}
std::string TestCase_084::title()
{
    return std::string("AI through commands: production (only what it can build), settler and swordman destinations, one order per decision.");

}

bool TestCase_084::done()
{
    return isdone;
}
bool TestCase_084::passed()
{
    return haspassed;
}
std::string TestCase_084::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_084();
}
