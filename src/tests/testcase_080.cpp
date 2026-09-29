//  TestCase_080.cpp
//  bunmei
//
//  Created by faturita on 29/09/2026
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
#include "../resources.h"
#include "../coordinator.h"
#include "../commandorder.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../diplomacy.h"
#include "../messages.h"
#include "../dee.h"
#include "../technologies.h"

#include "testcase_080.h"

// Factions are a dynamic structure. A new civilization (NewFactionOrder, what /newciv pushes)
// gets the next id and is added at the END of the turn order, with a civilization not in play,
// its starting units on free land, and a row in the diplomacy table and the tech tree. A
// faction with no cities and no units left is lost (RemoveFactionOrder, pushed when its last
// unit dies): it leaves the turn order, its land goes free and the turn passes on.

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
extern std::vector<Message> messages;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_080::TestCase_080()
{

}

TestCase_080::~TestCase_080()
{

}

int TestCase_080::number()
{
    return 80;
}

static int addWarrior(int faction, int lat, int lon)
{
    Warrior *w = new Warrior();
    w->id = getNextUnitId(); w->faction = faction;
    w->latitude = lat; w->longitude = lon;
    w->availablemoves = w->getUnitMoves();
    units[w->id] = w;
    map.set(lat,lon).setOwnedBy(faction);
    return w->id;
}

static bool hasMessage(const char* text)
{
    for (auto& m : messages)
        if (m.msg.find(text) != std::string::npos)
            return true;
    return false;
}

void TestCase_080::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initMovementCosts(movementcosts);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
            map.set(lat,lon).bioma = GRASSLAND;
            map.set(lat,lon).setVisible(0);
        }

    // Vikings, Romans, Greeks: the first three civilizations, as a -civs 3 game starts.
    for (int f=0;f<3;f++)
    {
        Faction *faction = createFaction(f);
        faction->autoPlayer = true;          // research is rolled, no selector dialog opens
        factions.push_back(faction);
        citynames[factions.nextId()-1] = std::queue<std::string>();
    }

    initDiplomacy(diplomacy, (int)factions.size());
    initTechnologies(techtree, (int)factions.size(), dee);

    // Vikings hold a city, Romans and Greeks one warrior each.
    City *city = new City(&map, 0, getNextCityId(), 0, 0);
    city->setName("Oslo");
    city->foundedyear = -4000;
    cities[city->id] = city;

    romanwarrior = addWarrior(1, 5, 5);
    addWarrior(2, -5, -5);

    // Land the Romans hold with nothing on it (a unit walked away, culture, ...).
    map.set(6,6).setOwnedBy(1);

    diplomacy[0][1].status = PEACE;

    mapzoom = 2;
    centermapinmap(0,0);

    // It is the Romans' turn, and the player is watching them.
    coordinator.a_f_id = 1;
    coordinator.v_f_id = 1;
    coordinator.a_u_id = CONTROLLING_NONE;

}

int TestCase_080::check(int year)
{

    ticks++;

    if (isdone)
        return 0;

    if (ticks < 3)
        return 0;

    isdone = true;
    haspassed = false;
    char buf[256];

    // tester.cpp resets the turn to faction 0 after init(): the Romans' turn is set up here.
    coordinator.a_f_id = 1;
    coordinator.v_f_id = 1;

    if (factions.size() != 3 || factions[0] == nullptr || factions[1] == nullptr || factions[2] == nullptr)
    {
        message = std::string("setup: push_back did not give the three factions ids 0, 1, 2.");
        return 0;
    }

    // ---- A new civilization arises -------------------------------------------------------
    CommandOrder co;
    co.command = Command::NewFactionOrder;
    co.parameters.factionid = 1;
    coordinator.push(co);
    processCommandOrders();

    if (factions.size() != 4 || !factions.has(3))
    {
        sprintf(buf, "NewFactionOrder: %zu factions, expected 4 with the new one as id 3.", factions.size());
        message = std::string(buf);
        return 0;
    }
    Faction *newciv = factions[3];
    if (factions.next(2) != 3 || factions.next(3) != -1)
    {
        message = std::string("the new civilization is not at the END of the turn order.");
        return 0;
    }
    if (newciv->definition < 3)
    {
        sprintf(buf, "the new civilization is %s, a civilization already in play.", newciv->name);
        message = std::string(buf);
        return 0;
    }
    if (!hasMessage("new nation under the sun"))
    {
        message = std::string("no 'new nation under the sun' message.");
        return 0;
    }
    bool welcomed = false;
    for (auto& m : messages)
        if (m.faction == 3 && m.msg.find("our destiny is to build a great empire") != std::string::npos)
            welcomed = true;
    if (!welcomed)
    {
        message = std::string("the new civilization did not get the welcome message.");
        return 0;
    }

    int newunits = 0;
    coordinate start(0,0);
    for (auto& [k, u] : units)
        if (u->faction == 3)
        {
            newunits++;
            start = coordinate(u->latitude, u->longitude);
        }
    if (newunits != 3)
    {
        sprintf(buf, "the new civilization has %d units, expected its 3 starting units.", newunits);
        message = std::string(buf);
        return 0;
    }
    if (!map.peek(start.lat, start.lon).isOwnedBy(3) || !map.peek(start.lat, start.lon).isVisible(3))
    {
        message = std::string("the new civilization does not hold, or see, its starting tile.");
        return 0;
    }

    // Its rows in the per-id tables, and the relations already there are kept.
    if (diplomacy[3][0].status != NO_CONTACT || diplomacy[2][3].status != NO_CONTACT)
    {
        message = std::string("the new civilization does not start at NO_CONTACT with everybody.");
        return 0;
    }
    if (diplomacy[0][1].status != PEACE)
    {
        message = std::string("growing the diplomacy table lost the Vikings-Romans PEACE.");
        return 0;
    }
    if (techtree.factionCount() != 4 || !techtree.graph(3).isDiscovered(TECH_ROOT) ||
        !dee.verifyDep(factionContext(3), techtree.graph(3).getTech(TECH_ROOT)->depCode))
    {
        message = std::string("the new civilization has no tech graph knowing the root.");
        return 0;
    }

    // ---- A civilization is lost ------------------------------------------------------------
    // A faction that still has a city is not lost, whoever says so.
    co.command = Command::RemoveFactionOrder;
    co.parameters.factionid = 0;
    coordinator.push(co);
    processCommandOrders();
    if (!factions.has(0))
    {
        message = std::string("RemoveFactionOrder removed the Vikings, who still have a city.");
        return 0;
    }

    // The Romans' last unit dies (cleanUnits erases it, as after a lost battle).
    units[romanwarrior]->markForDeletion();
    cleanUnits();
    processCommandOrders();

    if (factions.has(1) || factions.size() != 3)
    {
        message = std::string("the Romans lost their last unit and are still a faction.");
        return 0;
    }
    if (!hasMessage("Romans civilization has been lost, but his leader may return."))
    {
        message = std::string("no 'civilization has been lost' message.");
        return 0;
    }
    if (!map.peek(6,6).isFreeLand() || !map.peek(5,5).isFreeLand())
    {
        message = std::string("the Romans' land did not go free.");
        return 0;
    }
    if (coordinator.a_f_id != 2 || coordinator.v_f_id != 2)
    {
        sprintf(buf, "the turn did not pass from the lost Romans to the Greeks: a_f_id %d v_f_id %d.",
                coordinator.a_f_id, coordinator.v_f_id);
        message = std::string(buf);
        return 0;
    }
    if (factions.first() != 0 || factions.next(0) != 2 || factions.next(2) != 3)
    {
        message = std::string("turn order after the loss is not Vikings, Greeks, the new civilization.");
        return 0;
    }

    // Ids are never reused: the next civilization is id 4, not the Romans' 1.
    co.command = Command::NewFactionOrder;
    coordinator.push(co);
    processCommandOrders();
    if (!factions.has(4) || factions.has(1) || factions.size() != 4)
    {
        message = std::string("the next civilization did not get id 4.");
        return 0;
    }
    if (factions.next(3) != 4)
    {
        message = std::string("the second new civilization is not at the end of the turn order.");
        return 0;
    }

    haspassed = true;

    return 0;
}
std::string TestCase_080::title()
{
    return std::string("Factions: a new civilization gets the next id at the end; a lost one leaves, its land goes free.");

}

bool TestCase_080::done()
{
    return isdone;
}
bool TestCase_080::passed()
{
    return haspassed;
}
std::string TestCase_080::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_080();
}
