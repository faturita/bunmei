//  TestCase_072.cpp
//  bunmei
//
//  Created by Claude on 22/09/2026
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <vector>
#include <string>

#include "../map.h"
#include "../units/Unit.h"
#include "../units/Warrior.h"
#include "../City.h"
#include "../Faction.h"
#include "../coordinator.h"
#include "../commandorder.h"
#include "../engine.h"
#include "../tiles.h"
#include "../codes.h"
#include "../dee.h"
#include "../diplomacy.h"
#include "../usercontrols.h"

#include "testcase_072.h"

// @Task: the last three things in usercontrols.cpp that changed global game state directly
// now go through the command queue -- /autoplayer (Faction::autoPlayer), the peace/war dialog
// (the shared diplomacy table) and /enable (the DEE registry).
//
// Each is checked the same way, and that shape is the point:
//   * the CLIENT side refuses malformed or impossible input without pushing anything;
//   * a well-formed request only PUSHES -- nothing changes until processCommandOrders();
//   * the SERVER side re-checks everything the client checked, because a pushed order is the
//     only thing the handler can see and a remote client's word is not evidence. Those
//     duplicate checks are deliberate (faturita: "if you need to verify twice the same thing
//     do it"), so they are driven here by pushing orders DIRECTLY, bypassing the client
//     guardrails entirely -- which is exactly what a remote client could do.
//
// The diplomacy one matters most: it is the only command that writes state belonging to two
// factions at once, from what used to be a UI callback.

extern std::unordered_map<int, std::string> tiles;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Map map;
extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;
extern DependencyEvaluationEngine dee;
extern DiplomacyTable diplomacy;

#define TEST_MAPSIZE 1

#define WORLD_CODE   0x42
#define FACTION_CODE 0x43
#define CITY_CODE    0x44
#define UNUSED_CODE  0x45

// Types a teletype line and presses Enter, through the real keypress handler.
static void typeTeletype(const char* line)
{
    controller.teletype = true;
    controller.str.clear();
    for (const char* p = line; *p; p++)
        handleKeypress((unsigned char)*p, 0, 0);
    handleKeypress(13, 0, 0);
}

TestCase_072::TestCase_072() {}
TestCase_072::~TestCase_072() {}

int TestCase_072::number() { return 72; }

void TestCase_072::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initProductionRates(productionrates);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            mapcell &cell = map.set(lat,lon);
            cell = mapcell(LAND);
            cell.bioma = GRASSLAND;
            cell.setVisible(0);
            cell.setVisible(1);
        }
    assignProductionRates(map);

    for (int f=0;f<2;f++)
    {
        Faction *faction = new Faction();
        faction->id = f;
        strcpy(faction->name, f == 0 ? "Vikings" : "Romans");
        faction->red = 255; faction->green = 0; faction->blue = 0;
        faction->autoPlayer = false;
        factions.push_back(faction);
        citynames[f] = std::queue<std::string>();
    }

    initDiplomacy(diplomacy, (int)factions.size());

    City *city = new City(&map, 0, getNextCityId(), 0, 0);
    city->setName("Kattegate");
    cities[city->id] = city;
    cityid = city->id;

    // Two units within 5 tiles of each other, one per faction: what the 'k' diplomacy key
    // needs to find a negotiating partner.
    Warrior* mine = new Warrior();
    mine->id = getNextUnitId(); mine->faction = 0;
    mine->latitude = 2; mine->longitude = 2;
    mine->availablemoves = mine->getUnitMoves();
    units[mine->id] = mine;

    Warrior* theirs = new Warrior();
    theirs->id = getNextUnitId(); theirs->faction = 1;
    theirs->latitude = 3; theirs->longitude = 3;
    theirs->availablemoves = theirs->getUnitMoves();
    units[theirs->id] = theirs;

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = mine->id;
    controller.view = 1;
}

int TestCase_072::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };
    isdone = true;

    Faction* me    = factions[0];
    Faction* them  = factions[1];

    // ================= 1) /autoplayer ===================================================
    {
        me->autoPlayer = false;

        typeTeletype("/autoplayer on");
        if (me->autoPlayer)
        { fail("/autoplayer changed the faction inline -- it must only push a Command::SetAutoPlayerOrder."); return 0; }

        processCommandOrders();
        if (!me->autoPlayer)
        { fail("processCommandOrders() did not turn autoPlayer on for Command::SetAutoPlayerOrder."); return 0; }

        typeTeletype("/autoplayer off");
        processCommandOrders();
        if (me->autoPlayer)
        { fail("/autoplayer off did not turn autoPlayer back off."); return 0; }

        // Client guardrail: a line that names neither on nor off pushes nothing at all.
        me->autoPlayer = true;
        typeTeletype("/autoplayer sideways");
        processCommandOrders();
        if (!me->autoPlayer)
        { fail("A malformed /autoplayer line changed the faction -- it must not push anything."); return 0; }
        me->autoPlayer = false;

        // Server guardrail, pushed directly: a faction that does not exist is ignored, not
        // indexed into.
        CommandOrder co;
        co.command = Command::SetAutoPlayerOrder;
        co.parameters.factionid = 99;
        co.parameters.enabled   = true;
        coordinator.push(co);
        processCommandOrders();
        if (me->autoPlayer || them->autoPlayer)
        { fail("An order naming a nonexistent faction turned autoPlayer on for a real one."); return 0; }
    }

    // ================= 2) the peace/war dialog ==========================================
    {
        diplomacy[0][1].setStatus(NO_CONTACT);

        // The real key: it must raise the dialog and change NOTHING yet.
        handleKeypress('k', 0, 0);
        if (!controller.query.active)
        { fail("The 'k' key did not raise the peace/war dialog for a faction with a neighbour in range."); return 0; }
        if (diplomacy[0][1].status != NO_CONTACT)
        { fail("Raising the dialog already changed the diplomatic status."); return 0; }

        // Answering "Yes." must only PUSH -- this is the callback that used to write the
        // shared table from inside the UI.
        controller.query.selected(0);
        controller.query.active = false;
        if (diplomacy[0][1].status != NO_CONTACT)
        { fail("Answering the dialog wrote the diplomacy table directly -- it must only push a Command::SetDiplomacyOrder."); return 0; }

        processCommandOrders();
        if (diplomacy[0][1].status != PEACE)
        { fail("processCommandOrders() did not set PEACE for Command::SetDiplomacyOrder."); return 0; }
        // The table is undirected: one entry, both directions, and the flags follow the status.
        if (diplomacy[1][0].status != PEACE)
        { fail("The relation is undirected -- setting it one way must set it both ways."); return 0; }
        if (diplomacy[0][1].landSeizure)
        { fail("PEACE must clear landSeizure -- setStatus() has to re-derive the flags."); return 0; }

        // And "No." -> war, through the same path.
        handleKeypress('k', 0, 0);
        controller.query.selected(1);
        controller.query.active = false;
        processCommandOrders();
        if (diplomacy[0][1].status != FOE)
        { fail("Answering 'No.' did not put the two factions at war."); return 0; }

        // Server guardrails, pushed directly -- none of these can be produced by the dialog,
        // which is the point: a remote client is not the dialog.
        auto pushDiplomacy = [&](int a, int b, int status)
        {
            CommandOrder co;
            co.command = Command::SetDiplomacyOrder;
            co.parameters.factionid       = a;
            co.parameters.targetfactionid = b;
            co.parameters.status          = status;
            coordinator.push(co);
            processCommandOrders();
        };

        diplomacy[0][1].setStatus(PEACE);

        pushDiplomacy(0, 99, FOE);            // no such faction
        if (diplomacy[0][1].status != PEACE)
        { fail("An order naming a nonexistent faction changed a real relation."); return 0; }

        // A faction with itself. Checked on diplomacy[0][0], NOT on [0][1]: the table is
        // indexed per unordered pair, so a self-relation lands in its own entry and would be
        // invisible from any other pair's.
        const int selfBefore = diplomacy[0][0].status;
        pushDiplomacy(0, 0, FOE);
        if (diplomacy[0][0].status != selfBefore)
        { fail("An order setting a faction's relation with ITSELF was applied."); return 0; }
        if (diplomacy[0][1].status != PEACE)
        { fail("A self-relation order disturbed a real relation."); return 0; }

        pushDiplomacy(0, 1, 77);              // not a DiplomaticStatus
        if (diplomacy[0][1].status != PEACE)
        { fail("An order carrying a status outside the DefCon table was applied."); return 0; }

        // A valid status that is neither peace nor war still works -- the command takes a
        // status, it is not a two-verb switch.
        pushDiplomacy(0, 1, TRADE_AGREEMENT);
        if (diplomacy[0][1].status != TRADE_AGREEMENT)
        { fail("A valid DiplomaticStatus other than PEACE/FOE was refused."); return 0; }
        if (!diplomacy[0][1].openBorders)
        { fail("TRADE_AGREEMENT opens borders -- the flags must follow the status."); return 0; }
    }

    // ================= 3) /enable =======================================================
    {
        typeTeletype("/enable world 0x42");
        if (dee.verifyDep(worldContext(), WORLD_CODE))
        { fail("/enable registered the code inline -- it must only push a Command::RegisterDependencyOrder."); return 0; }

        processCommandOrders();
        if (!dee.verifyDep(worldContext(), WORLD_CODE))
        { fail("processCommandOrders() did not register the world-scope code."); return 0; }

        typeTeletype("/enable faction 0x43");
        processCommandOrders();
        if (!dee.verifyDep(factionContext(0), FACTION_CODE))
        { fail("The faction-scope code was not registered for the calling faction."); return 0; }
        if (dee.verifyDep(factionContext(1), FACTION_CODE))
        { fail("The faction-scope code leaked onto another faction."); return 0; }

        typeTeletype("/enable city Kattegate 0x44");
        processCommandOrders();
        if (!dee.verifyDep(cityContext(cityid), CITY_CODE))
        { fail("The city-scope code was not registered for the named city."); return 0; }

        // Client guardrails: each of these must push nothing.
        typeTeletype("/enable city Nowhere 0x45");
        typeTeletype("/enable world zzzz");
        typeTeletype("/enable sideways 0x45");
        typeTeletype("/enable world");
        processCommandOrders();
        if (dee.verifyDep(worldContext(), UNUSED_CODE) || dee.verifyDep(factionContext(0), UNUSED_CODE))
        { fail("A malformed /enable line registered something anyway."); return 0; }

        // Server guardrails, pushed directly.
        auto pushDep = [&](int scope, int code, int factionid, int cityidParam)
        {
            CommandOrder co;
            co.command = Command::RegisterDependencyOrder;
            co.parameters.scope     = scope;
            co.parameters.codeid    = code;
            co.parameters.factionid = factionid;
            co.parameters.cityid    = cityidParam;
            coordinator.push(co);
            processCommandOrders();
        };

        pushDep(99, UNUSED_CODE, 0, cityid);            // unknown scope
        pushDep(DEP_SCOPE_WORLD, 0, 0, cityid);         // code 0 registers nothing
        pushDep(DEP_SCOPE_FACTION, UNUSED_CODE, 99, cityid);   // no such faction
        pushDep(DEP_SCOPE_CITY, UNUSED_CODE, 0, 99999);        // no such city

        if (dee.verifyDep(worldContext(), UNUSED_CODE) ||
            dee.verifyDep(factionContext(0), UNUSED_CODE) ||
            dee.verifyDep(cityContext(cityid), UNUSED_CODE))
        { fail("An order with an invalid scope, code, faction or city registered something."); return 0; }
        if (cities.find(99999) != cities.end() || dee.verifyDep(worldContext(), 0))
        { fail("An invalid order inserted a city or registered code 0."); return 0; }
    }

    haspassed = true;
    return 0;
}

std::string TestCase_072::title()
{
    return std::string("The last three direct global mutations in usercontrols.cpp are commands now: SetAutoPlayerOrder (Faction::autoPlayer), SetDiplomacyOrder (the shared, undirected diplomacy table, from what was a UI callback) and RegisterDependencyOrder (the DEE registry, via /enable). Each validates on the client AND re-validates in the handler, checked here by pushing malformed orders directly the way a remote client could.");
}

bool TestCase_072::done()   { return isdone; }
bool TestCase_072::passed() { return haspassed; }
std::string TestCase_072::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_072();
}
