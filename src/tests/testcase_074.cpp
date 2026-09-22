//  TestCase_074.cpp
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
#include "../units/Scout.h"
#include "../City.h"
#include "../Faction.h"
#include "../coordinator.h"
#include "../commandorder.h"
#include "../engine.h"
#include "../tiles.h"
#include "../codes.h"
#include "../dee.h"
#include "../technologies.h"
#include "../usercontrols.h"

#include "testcase_074.h"

// @Task: the last three places the UI reached into the model.
//
//   0. HOW FAR a unit sees is its own property (Unit::visionRange, 1 for everything ordinary
//      and 2 for a Scout) rather than a constant inside revealAround().
//
//   1. FOG OF WAR was advanced by the RENDERER -- map.cpp:drawUnitsAndCities() called unfog()
//      for every unit every frame, and nothing else ever revealed a tile. mapcell::visible is
//      per-faction and is SAVED with the map, so exploration was model state being produced as
//      a side effect of painting: the headless simulator uncovered nothing at all, and in the
//      game a tile became visible because it was drawn rather than because somebody walked
//      there. It is now engine.cpp:revealAround(), called from the movement path, plus
//      updateFogOfWar() once at setup and once a turn for units that appear without moving.
//
//   2. THE GOTO DESTINATION CLICK wrote Unit::goTo() directly. MoveUnitTo is a single STEP, so
//      there was no way at all for an AI or a remote player to say "head there".
//      -> Command::SetUnitDestinationOrder.
//
//   3. THE RESEARCH SELECTOR set techtree.setResearchTarget() from its dialog callback -- the
//      same shape the peace/war dialog had before it became SetDiplomacyOrder.
//      -> Command::SetResearchTargetOrder.
//
// Each is checked the same way as the earlier migrations: the client refuses what it should,
// a well-formed request only PUSHES, and the handler re-checks ownership/validity by being
// sent orders directly, the way a remote client could.

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
extern TechTree techtree;
extern bool goToMode;

extern int REAL_SCREEN_WIDTH;
extern int REAL_SCREEN_HEIGHT;

#define TEST_MAPSIZE 1

#define SCOUT_LAT   0
#define SCOUT_LON   0
#define ENEMY_LAT   4
#define ENEMY_LON   4
#define DEST_LAT   -3
#define DEST_LON    2

TestCase_074::TestCase_074() {}
TestCase_074::~TestCase_074() {}

int TestCase_074::number() { return 74; }

void TestCase_074::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);
    initProductionRates(productionrates);

    // Everything starts FOGGED for everybody -- mapcell's constructor clears `visible`, and
    // this test is about what uncovers it.
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            mapcell &cell = map.set(lat,lon);
            cell = mapcell(LAND);
            cell.bioma = GRASSLAND;
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

    initTechnologies(techtree, (int)factions.size(), dee);

    City *city = new City(&map, 0, getNextCityId(), 0, 6);
    city->setName("Kattegate");
    cities[city->id] = city;
    cityid = city->id;

    Scout* scout = new Scout();
    scout->id = getNextUnitId(); scout->faction = 0;
    scout->latitude = SCOUT_LAT; scout->longitude = SCOUT_LON;
    scout->availablemoves = scout->getUnitMoves();
    units[scout->id] = scout;
    scoutid = scout->id;

    Warrior* enemy = new Warrior();
    enemy->id = getNextUnitId(); enemy->faction = 1;
    enemy->latitude = ENEMY_LAT; enemy->longitude = ENEMY_LON;
    enemy->availablemoves = enemy->getUnitMoves();
    units[enemy->id] = enemy;
    enemyid = enemy->id;

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = scout->id;
    controller.view = 1;
}

int TestCase_074::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };
    isdone = true;

    Unit* scout = units[scoutid];

    // ================= 1) fog of war is a model rule ====================================
    {
        // Wipe every tile's visibility: only the model may put it back.
        for(int lat=map.minlat;lat<map.maxlat;lat++)
            for (int lon=map.minlon;lon<map.maxlon;lon++)
                map.set(lat,lon).visible.clear();

        if (map.peek(SCOUT_LAT,SCOUT_LON).isVisible(0))
        { fail("Setup: the map should start fogged."); return 0; }

        // Drawing must NOT reveal anything any more. drawUnitsAndCities() is the function
        // that used to do it, so this is the regression that matters.
        drawUnitsAndCities();
        if (map.peek(SCOUT_LAT,SCOUT_LON).isVisible(0))
        { fail("Rendering revealed the map -- fog of war must be advanced by the model, not by drawing."); return 0; }

        // Standing somewhere reveals it: the per-turn sweep, for units that never moved.
        updateFogOfWar();
        if (!map.peek(SCOUT_LAT,SCOUT_LON).isVisible(0))
        { fail("updateFogOfWar() did not reveal the tile a unit is standing on."); return 0; }
        if (!map.peek(SCOUT_LAT+1,SCOUT_LON).isVisible(0) || !map.peek(SCOUT_LAT,SCOUT_LON-1).isVisible(0))
        { fail("A unit should reveal the tiles around it, not only its own."); return 0; }

        // How far a unit sees is ITS OWN property (Unit::visionRange), not a constant in
        // revealAround(). A Scout is the unit that exists to see, and sees twice as far.
        if (scout->getVisionRange() != 2)
        { fail("A Scout should have a vision range of 2."); return 0; }
        if (!map.peek(SCOUT_LAT+2,SCOUT_LON).isVisible(0) || !map.peek(SCOUT_LAT-2,SCOUT_LON+2).isVisible(0))
        { fail("A Scout sees two tiles out -- including the corners of that square."); return 0; }
        if (map.peek(SCOUT_LAT+3,SCOUT_LON).isVisible(0))
        { fail("A Scout revealed further than its vision range."); return 0; }

        // ...while an ordinary unit still sees exactly one tile out. Same code path, so this
        // is what proves the radius comes from the unit rather than being hardcoded.
        Unit* ordinary = units[enemyid];
        if (ordinary->getVisionRange() != 1)
        { fail("An ordinary unit should have a vision range of 1."); return 0; }
        if (!map.peek(ENEMY_LAT+1,ENEMY_LON+1).isVisible(1))
        { fail("An ordinary unit should reveal the ring around it."); return 0; }
        if (map.peek(ENEMY_LAT+2,ENEMY_LON).isVisible(1))
        { fail("An ordinary unit revealed two tiles out -- only a Scout sees that far."); return 0; }

        // Per FACTION: the Vikings' scout does not uncover the map for the Romans.
        if (map.peek(SCOUT_LAT,SCOUT_LON).isVisible(1))
        { fail("One faction's unit revealed the tile for ANOTHER faction -- fog is per faction."); return 0; }
        if (!map.peek(ENEMY_LAT,ENEMY_LON).isVisible(1))
        { fail("The Romans' own unit should have revealed its own surroundings."); return 0; }

        // And MOVING reveals: the movement path, which is where exploration belongs.
        // One step forward puts the far edge of the scout's sight on SCOUT_LAT+3, which is
        // still fogged right now (its range is 2).
        const int aheadLat = SCOUT_LAT+3, aheadLon = SCOUT_LON;
        if (map.peek(aheadLat,aheadLon).isVisible(0))
        { fail("Setup: the tile beyond the scout's sight should still be fogged."); return 0; }

        moveUnit(scout, SCOUT_LAT+1, SCOUT_LON);
        if (!map.peek(aheadLat,aheadLon).isVisible(0))
        { fail("Moving did not reveal the ground ahead -- revealAround() must run on the movement path."); return 0; }
    }

    // ---- the very first frame must not be black -------------------------------------------
    // @Issue (reported live): year -4000 rendered completely black and nothing could be done
    // until the player pressed space, after which the units appeared. The fog reveal that
    // used to happen as a side effect of the FIRST DRAW had moved into the model, but its
    // setup call landed in switchFaction() -- which only runs when the turn passes to a
    // faction -- so at world-setup time nothing had revealed anything yet. It belongs in the
    // world-setup path, before a single frame is drawn.
    //
    // Checked as the invariant rather than the call site: after setup and before ANY turn has
    // passed, a faction can see the ground its own starting units stand on.
    {
        for(int lat=map.minlat;lat<map.maxlat;lat++)
            for (int lon=map.minlon;lon<map.maxlon;lon++)
                map.set(lat,lon).visible.clear();

        // Exactly what setupWorldModelling()/main() do once the world exists -- no endOfYear(),
        // no switchFaction(), no processCommandOrders(): just a world that has been built.
        updateFogOfWar();

        for (auto& [k,u] : units)
            if (!map.peek(u->latitude,u->longitude).isVisible(u->faction))
            {
                char buf[220];
                snprintf(buf,sizeof(buf),"Unit %d cannot see its own tile before the first turn -- the map would "
                                         "render black until something ended the year.", u->id);
                fail(buf); return 0;
            }
    }

    // ================= 2) the GoTo destination click ====================================
    {
        scout->resetGoTo();
        if (scout->isAuto())
        { fail("Setup: the scout should not be automated yet."); return 0; }

        // The real path: 'G' arms goToMode, then a map click picks the destination.
        goToMode = false;
        handleKeypress('G', 0, 0);
        if (!goToMode)
        { fail("The GoTo key did not arm the destination click."); return 0; }

        coordinate cs = map.to_screen(DEST_LAT, DEST_LON);
        centermapinmap(cs.lat, cs.lon);
        processMouse(GLUT_LEFT_BUTTON, GLUT_DOWN, REAL_SCREEN_WIDTH/2, REAL_SCREEN_HEIGHT/2);

        if (scout->isAuto())
        { fail("The destination click set the unit's target directly -- it must only push a Command::SetUnitDestinationOrder."); return 0; }

        processCommandOrders();

        if (!scout->isAuto())
        { fail("processCommandOrders() did not automate the unit for Command::SetUnitDestinationOrder."); return 0; }
        if (scout->target.lat != DEST_LAT || scout->target.lon != DEST_LON)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"The unit was sent to (%d,%d), the click chose (%d,%d).",
                     scout->target.lat, scout->target.lon, DEST_LAT, DEST_LON);
            fail(buf); return 0;
        }

        // Server guardrail: a faction cannot send somebody else's unit.
        Unit* enemy = units[enemyid];
        enemy->resetGoTo();

        CommandOrder co;
        co.command = Command::SetUnitDestinationOrder;
        co.parameters.spawnid   = enemyid;
        co.parameters.factionid = 0;          // faction 0 ordering faction 1's unit
        co.parameters.latitude  = DEST_LAT;
        co.parameters.longitude = DEST_LON;
        coordinator.push(co);
        processCommandOrders();

        if (enemy->isAuto())
        { fail("A faction sent a unit belonging to ANOTHER faction -- the handler must check ownership."); return 0; }

        // And a unit that does not exist is ignored rather than inserted.
        CommandOrder gone;
        gone.command = Command::SetUnitDestinationOrder;
        gone.parameters.spawnid   = 99999;
        gone.parameters.factionid = 0;
        gone.parameters.latitude  = DEST_LAT;
        gone.parameters.longitude = DEST_LON;
        coordinator.push(gone);
        processCommandOrders();

        if (units.find(99999) != units.end())
        { fail("Looking up a missing unit id inserted it -- use find(), never units[id]."); return 0; }
    }

    // ================= 3) the research selector =========================================
    {
        // Every faction starts knowing the root only, so the Frontier is exactly {Language}.
        std::vector<int> frontier = techtree.graph(0).getFrontierOrdered();
        if (frontier.empty())
        { fail("Setup: faction 0 should have a Frontier to choose from."); return 0; }

        techtree.setResearchTarget(0, 0);     // clear whatever is selected
        controller.query.active = false;

        chooseResearch(0, true);
        if (!controller.query.active)
        { fail("chooseResearch() did not raise the selector for a human faction."); return 0; }
        if (techtree.getResearchTarget(0) != 0)
        { fail("Raising the selector already set a research target."); return 0; }

        // Answering must only PUSH -- this is the callback that used to write the techtree.
        controller.query.selected(0);
        controller.query.active = false;
        if (techtree.getResearchTarget(0) != 0)
        { fail("Answering the selector set the research target directly -- it must only push a Command::SetResearchTargetOrder."); return 0; }

        processCommandOrders();
        if (techtree.getResearchTarget(0) != frontier[0])
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"After processing, the target is 0x%x; the chosen technology was 0x%x.",
                     techtree.getResearchTarget(0), frontier[0]);
            fail(buf); return 0;
        }

        // Server guardrails, pushed directly.
        auto pushResearch = [&](int factionid, int techid)
        {
            CommandOrder co;
            co.command = Command::SetResearchTargetOrder;
            co.parameters.factionid = factionid;
            co.parameters.techid    = techid;
            coordinator.push(co);
            processCommandOrders();
        };

        const int keep = techtree.getResearchTarget(0);

        // A real technology that is NOT in this faction's Frontier: you cannot research what
        // you do not know yet, and that rule has to hold for a pushed order too.
        pushResearch(0, TECH_INDUSTRIALIZATION);
        if (techtree.getResearchTarget(0) != keep)
        { fail("A technology outside the faction's Frontier was accepted as a research target."); return 0; }

        pushResearch(0, 0x7fff);              // not a technology at all
        if (techtree.getResearchTarget(0) != keep)
        { fail("An id that is not a technology was accepted."); return 0; }

        pushResearch(99, frontier[0]);        // no such faction
        if (techtree.getResearchTarget(0) != keep)
        { fail("An order naming a nonexistent faction changed a real faction's research."); return 0; }
    }

    haspassed = true;
    return 0;
}

std::string TestCase_074::title()
{
    return std::string("The last three UI->model writes are gone: fog of war is advanced by engine.cpp:revealAround() from the movement path instead of by drawUnitsAndCities() (so the headless simulator explores too, and drawing reveals nothing), the GoTo destination click pushes Command::SetUnitDestinationOrder, and the research selector pushes Command::SetResearchTargetOrder. Each handler re-checks ownership/validity against orders pushed directly.");
}

bool TestCase_074::done()   { return isdone; }
bool TestCase_074::passed() { return haspassed; }
std::string TestCase_074::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_074();
}
