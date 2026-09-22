//  TestCase_071.cpp
//  bunmei
//
//  Created by Claude on 22/09/2026
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <vector>

#include "../map.h"
#include "../units/Unit.h"
#include "../units/Warrior.h"
#include "../units/Worker.h"
#include "../City.h"
#include "../Faction.h"
#include "../coordinator.h"
#include "../commandorder.h"
#include "../engine.h"
#include "../tiles.h"
#include "../improvements.h"
#include "../usercontrols.h"

#include "testcase_071.h"

// @Task: selecting a unit on the map went straight from usercontrols.cpp into
// engine.cpp:activateUnit(). It is a game action, so it now goes through the queue like every
// other one: Command::ActivateUnitOrder.
//
// Why it matters beyond tidiness: activating is not a passive selection. It PACKS UP a
// fortified unit, WAKES a sentried one, and INTERRUPTS a working one (Unit::completed(), no
// finalize -- the effort restarts later). Those are real state changes, and the AI and a
// remote player have no way to make them except through a command.
//
// It also gains a check it did not have: the handler verifies the unit belongs to the faction
// claiming it. The old caller tested that too, but a caller's test is only a local
// convenience -- once the caller is a remote client it cannot be trusted, and selecting a
// unit would otherwise be the way into someone else's army.
//
// Driven through the REAL click path (processMouse at screen centre over the unit's tile,
// same technique as testcase_014/025) so the migration itself is what is tested: the click
// must NOT mutate anything by itself, and processCommandOrders() must.

extern std::unordered_map<int, std::string> tiles;
extern std::unordered_map<int, Improvement*> improvements;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern ImprovementEffort improvementeffort;
extern Map map;
extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

extern int REAL_SCREEN_WIDTH;
extern int REAL_SCREEN_HEIGHT;

#define TEST_MAPSIZE 1

#define WARRIOR_LAT  0
#define WARRIOR_LON  0
#define ENEMY_LAT    2
#define ENEMY_LON    0
#define WORKER_LAT  -2
#define WORKER_LON   0

TestCase_071::TestCase_071() {}
TestCase_071::~TestCase_071() {}

int TestCase_071::number() { return 71; }

void TestCase_071::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initProductionRates(productionrates);
    initImprovements(improvements);
    initImprovementEffort(improvementeffort);

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

    auto place = [&](Unit* u, int faction, int lat, int lon)
    {
        u->id = getNextUnitId();
        u->faction = faction;
        u->latitude = lat; u->longitude = lon;
        u->availablemoves = u->getUnitMoves();
        units[u->id] = u;
        map.set(lat,lon).setOwnedBy(faction);
        return u->id;
    };

    // Ours, fortified -- activating has to pack it up.
    warriorid = place(new Warrior(), 0, WARRIOR_LAT, WARRIOR_LON);
    units[warriorid]->fortify();

    // Somebody else's, sitting where we can click it.
    enemyid = place(new Warrior(), 1, ENEMY_LAT, ENEMY_LON);

    // Ours, mid-improvement: processWork() zeroes a working unit's moves every turn, so it
    // must stay selectable on isWorking() alone.
    workerid = place(new Worker(), 0, WORKER_LAT, WORKER_LON);
    units[workerid]->irrigating(getImprovementEffort(improvementeffort, IRRIGATION, GRASSLAND));
    units[workerid]->availablemoves = 0;

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;
    controller.view = 1;
}

int TestCase_071::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };
    isdone = true;

    // Clicks the map tile a unit stands on, through the real mouse handler.
    auto clickTile = [&](int lat, int lon)
    {
        coordinate cs = map.to_screen(lat, lon);
        centermapinmap(cs.lat, cs.lon);
        processMouse(GLUT_LEFT_BUTTON, GLUT_DOWN, REAL_SCREEN_WIDTH/2, REAL_SCREEN_HEIGHT/2);
    };

    // ---- 1) the click pushes a command and changes NOTHING by itself ----------------------
    {
        coordinator.a_u_id = CONTROLLING_NONE;
        clickTile(WARRIOR_LAT, WARRIOR_LON);

        if (coordinator.a_u_id == warriorid)
        { fail("The map click activated the unit directly -- it must only push a Command::ActivateUnitOrder now."); return 0; }
        if (!units[warriorid]->isFortified())
        { fail("The map click packed the unit up directly, without going through the command."); return 0; }

        processCommandOrders();

        if (coordinator.a_u_id != warriorid)
        { fail("processCommandOrders() did not make the clicked unit active for Command::ActivateUnitOrder."); return 0; }
        if (units[warriorid]->isFortified())
        { fail("Activating a fortified unit must pack it up."); return 0; }
    }

    // ---- 2) it wakes a sentried unit and interrupts a working one -------------------------
    {
        units[warriorid]->sentry();
        coordinator.a_u_id = CONTROLLING_NONE;

        CommandOrder co;
        co.command = Command::ActivateUnitOrder;
        co.parameters.spawnid   = warriorid;
        co.parameters.factionid = 0;
        coordinator.push(co);
        processCommandOrders();

        if (units[warriorid]->isSentry())
        { fail("Activating a sentried unit must wake it up."); return 0; }

        // The worker has 0 moves because it is WORKING -- it must still be activatable, and
        // activating must interrupt the improvement.
        if (!units[workerid]->isWorking())
        { fail("Setup: the worker should be mid-improvement."); return 0; }

        CommandOrder wco;
        wco.command = Command::ActivateUnitOrder;
        wco.parameters.spawnid   = workerid;
        wco.parameters.factionid = 0;
        coordinator.push(wco);
        processCommandOrders();

        if (coordinator.a_u_id != workerid)
        { fail("A working unit has 0 moves but must still be selectable -- processWork() zeroes them every turn."); return 0; }
        if (units[workerid]->isWorking())
        { fail("Activating a working unit must interrupt its improvement."); return 0; }
    }

    // ---- 3) a unit of ANOTHER faction is refused ------------------------------------------
    // The handler's own ownership check, not the caller's: this pushes the order directly,
    // the way a remote client could, bypassing usercontrols.cpp's local test entirely.
    {
        coordinator.a_u_id = CONTROLLING_NONE;
        units[enemyid]->fortify();

        CommandOrder co;
        co.command = Command::ActivateUnitOrder;
        co.parameters.spawnid   = enemyid;
        co.parameters.factionid = 0;          // faction 0 claiming faction 1's unit
        coordinator.push(co);
        processCommandOrders();

        if (coordinator.a_u_id == enemyid)
        { fail("A faction activated a unit belonging to ANOTHER faction -- the handler must check ownership itself."); return 0; }
        if (!units[enemyid]->isFortified())
        { fail("The refused activation still packed up the other faction's unit."); return 0; }
    }

    // ---- 4) a unit with nothing left to do is refused, and a missing one is ignored -------
    {
        coordinator.a_u_id = CONTROLLING_NONE;
        units[warriorid]->fortify();
        units[warriorid]->availablemoves = 0;   // spent, and not working

        CommandOrder co;
        co.command = Command::ActivateUnitOrder;
        co.parameters.spawnid   = warriorid;
        co.parameters.factionid = 0;
        coordinator.push(co);
        processCommandOrders();

        if (coordinator.a_u_id == warriorid)
        { fail("A unit with no moves left and no work in progress must not become active."); return 0; }
        if (!units[warriorid]->isFortified())
        { fail("The refused activation still packed the unit up."); return 0; }

        CommandOrder gone;
        gone.command = Command::ActivateUnitOrder;
        gone.parameters.spawnid   = 99999;
        gone.parameters.factionid = 0;
        coordinator.push(gone);
        processCommandOrders();

        if (units.find(99999) != units.end())
        { fail("Looking up a missing unit id inserted it -- use find(), never units[id]."); return 0; }
    }

    haspassed = true;
    return 0;
}

std::string TestCase_071::title()
{
    return std::string("Map unit selection migrated to Command::ActivateUnitOrder: the click only pushes, processCommandOrders() does the work (packs up a fortified unit, wakes a sentried one, interrupts a working one), and the handler checks the unit's OWNERSHIP itself rather than trusting the caller -- so the AI and a remote player can select a unit, and cannot select somebody else's.");
}

bool TestCase_071::done()   { return isdone; }
bool TestCase_071::passed() { return haspassed; }
std::string TestCase_071::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_071();
}
