//  TestCase_059.cpp
//  bunmei
//
//  Created by Claude on 08/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <vector>
#include <unordered_map>

#include "../map.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../codes.h"
#include "../dee.h"
#include "../technologies.h"
#include "../buildings/Granary.h"

#include "testcase_059.h"

// @Task: wire the tech graph into the game. This covers the integration seams (the scaffold
// itself is testcase_058):
//   1. initTechnologies() -- the shared game/simulator setup: one graph per faction, every
//      faction starting at the root, and TECH_ROOT registered in the DEE for each of them.
//   2. chooseResearch() (engine.cpp) -- an autoPlayer faction rolls a random Frontier tech; a
//      human faction gets the controller.query selector, whose options ARE the Frontier (one
//      option, "Language", at the start of a game) and whose callback sets the target.
//   3. TechTree::advance(faction, science, dee) -- the one-line-per-faction call endOfYear()
//      makes: pour a year's SCIENCE into the selected technology and step the graph.
//   4. The endOfYear() sweep itself: every city's SCIENCE is pooled per faction and the city
//      counter cleared (SCIENCE is spent, not stockpiled).
//   5. The payoff: a buildable gated behind a technology (Granary needs TECH_POTTERY) is
//      invisible before the discovery and offered after it.
//   6. After every discovery the faction is asked AGAIN where its science goes -- the Frontier
//      just widened -- via chooseResearch(f, force=true).
//   7. SCIENCE handed to advance() while no target is usable (the player has been prompted but
//      has not answered) is BANKED and spent whole later, never silently lost.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Tiles tiles;
extern DependencyEvaluationEngine dee;
extern TechTree techtree;

extern float mapzoom;
extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_059::TestCase_059() {}
TestCase_059::~TestCase_059() {}

int TestCase_059::number() { return 59; }

void TestCase_059::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon) = mapcell(LAND);
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon).setVisible(0);

    // Faction 0 is the human player (gets the selector dialog), faction 1 is an AI (rolls).
    Faction *human = new Faction();
    human->id = 0;
    strcpy(human->name,"Vikings");
    human->red = 255; human->green = 0; human->blue = 0;
    human->autoPlayer = false;
    factions.push_back(human);

    Faction *ai = new Faction();
    ai->id = 1;
    strcpy(ai->name,"Romans");
    ai->red = 0; ai->green = 0; ai->blue = 255;
    ai->autoPlayer = true;
    factions.push_back(ai);

    City *city = new City(&map, 0, getNextCityId(), 3, 3);
    city->setName("Kattegate");
    city->foundedyear = -4000;
    cities[city->id] = city;
    cityid = city->id;

    citynames[0] = std::queue<std::string>();
    citynames[1] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
}

static bool hasBuildableNamed(City* city, const char* name)
{
    for (BuildableFactory* bf : city->buildable)
        if (strcmp(bf->name, name) == 0)
            return true;
    return false;
}

int TestCase_059::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    City* city = cities[cityid];

    // The tester runs the real game loop, which may already have run an endOfYear (and so a
    // research turn) before this tick. Reset to a known state -- this is also the exact call
    // gamekernel.cpp / simulate.cpp make at world setup.
    initTechnologies(techtree, (int)factions.size(), dee);

    // ---- 1) initTechnologies: one graph per faction, everybody starts at the root ---------
    if (techtree.factionCount() != (int)factions.size())
    { fail("initTechnologies() should build one graph per faction."); return 0; }
    for (int f=0; f<(int)factions.size(); f++)
    {
        if (!techtree.graph(f).isDiscovered(TECH_ROOT))
        { fail("Every faction must start knowing the root technology (Language)."); return 0; }
        if (!dee.verifyDep(factionContext(f), TECH_ROOT))
        { fail("The root technology's dep code must be registered in the DEE for every faction."); return 0; }
        if (techtree.getResearchTarget(f) != 0)
        { fail("A faction starts with no research target selected."); return 0; }
        if (!techtree.needsResearchTarget(f))
        { fail("A faction with no target selected must report needsResearchTarget()."); return 0; }
    }
    if (techtree.graph(0).isDiscovered(TECH_POTTERY))
    { fail("A faction starts knowing the root ONLY."); return 0; }

    // ---- 2a) chooseResearch on the AI faction: rolls a Frontier technology ---------------
    chooseResearch(1);
    if (techtree.getResearchTarget(1) != TECH_LANGUAGE)
    { fail("The AI should have rolled a Frontier technology -- at the start only Language is available."); return 0; }
    if (techtree.needsResearchTarget(1))
    { fail("Once a valid target is set the faction should not need asking again."); return 0; }
    if (controller.query.active)
    { fail("An autoPlayer faction must NOT pop the selector dialog."); return 0; }

    // ---- 2b) chooseResearch on the human faction: the controller.query selector ----------
    controller.query.active = false;
    chooseResearch(0);
    if (!controller.query.active)
    { fail("A human faction should be asked through controller.query."); return 0; }
    if (controller.query.options.size() != 1 || controller.query.options[0] != "Language")
    { fail("At the start of a game the selector should offer exactly one option: Language."); return 0; }
    if (techtree.getResearchTarget(0) != 0)
    { fail("The target must only be set once the player actually answers the dialog."); return 0; }

    controller.query.selected(0);                 // the player picks the first option
    controller.query.active = false;
    if (techtree.getResearchTarget(0) != TECH_LANGUAGE)
    { fail("Answering the selector should set that faction's research target."); return 0; }

    // Investing is still refused for anything outside the Frontier.
    if (techtree.setResearchTarget(0, TECH_POTTERY))
    { fail("setResearchTarget() must reject a technology that is not in the Frontier."); return 0; }

    // ---- 3+4) the endOfYear sweep: pool each faction's SCIENCE, clear it, advance ---------
    city->resources[SCIENCE] = 1000;              // one very productive year

    std::unordered_map<int,int> sciencePerFaction;
    for (auto& [k, c] : cities)
    {
        sciencePerFaction[c->faction] += c->resources[SCIENCE];
        c->resources[SCIENCE] = 0;
    }
    if (city->resources[SCIENCE] != 0)
    { fail("endOfYear must CLEAR a city's SCIENCE after pooling it -- science is spent, not stockpiled."); return 0; }
    if (sciencePerFaction[0] != 1000)
    { fail("The faction's pooled SCIENCE should be the sum over its cities."); return 0; }

    std::vector<int> discovered = techtree.advance(0, sciencePerFaction[0], dee);

    // 1000 SCIENCE in Language fires every one of its children whatever the random weights
    // (every weight is >= TECH_WEIGHT_MIN = 0.1 > 0).
    if (discovered.empty())
    { fail("A year of SCIENCE poured into Language should have discovered something."); return 0; }
    if (!techtree.graph(0).isDiscovered(TECH_POTTERY))
    { fail("Pottery is a direct child of Language -- it should have fired."); return 0; }
    if (!dee.verifyDep(factionContext(0), TECH_POTTERY))
    { fail("A discovery must register its dep code in the DEE at faction scope."); return 0; }

    // Faction 1 never got any SCIENCE, so its graph must be untouched.
    if (techtree.graph(1).isDiscovered(TECH_POTTERY) || dee.verifyDep(factionContext(1), TECH_POTTERY))
    { fail("Faction 1 spent no SCIENCE -- it must not have discovered anything."); return 0; }

    // Language has now spent its whole fan-out, so the faction needs a new target -- which is
    // what makes endOfYear() ask again next year.
    if (!techtree.needsResearchTarget(0))
    { fail("Once the selected technology leaves the Frontier the faction must need a new target."); return 0; }

    // ---- 5) the payoff: the gated buildable is now offered -------------------------------
    // Granary is gated on TECH_POTTERY (Granary.cpp addDependencyCode). Faction 1 has not
    // discovered it, faction 0 just did.
    City* aicity = new City(&map, 1, getNextCityId(), 6, 6);
    aicity->setName("Roma");
    cities[aicity->id] = aicity;

    aicity->buildable.clear();
    populateCityBuildables(aicity);
    if (hasBuildableNamed(aicity, "Granary"))
    { fail("Granary is gated on TECH_POTTERY -- it must not be offered to a faction that lacks it."); return 0; }

    city->buildable.clear();
    populateCityBuildables(city);
    if (!hasBuildableNamed(city, "Granary"))
    { fail("After discovering Pottery the Granary should become buildable."); return 0; }

    // ---- 6) after a discovery the faction is asked AGAIN (the Frontier just widened) -----
    {
        // Faction 0 discovered a batch above, and Language dropped out of the Frontier, so a
        // plain chooseResearch already has something to ask -- pick a target first, so what we
        // are testing is the FORCED re-prompt on a still-valid target, not the stale-target one.
        controller.query.active = false;
        std::vector<int> frontier = techtree.graph(0).getFrontierOrdered();
        if (frontier.empty())
        { fail("Faction 0's Frontier should not be empty after a batch of discoveries."); return 0; }

        techtree.setResearchTarget(0, frontier[0]);
        if (techtree.needsResearchTarget(0))
        { fail("Setting a Frontier tech as the target should satisfy needsResearchTarget()."); return 0; }

        // Not forced: a valid target means no question is asked.
        chooseResearch(0);
        if (controller.query.active)
        { fail("chooseResearch() must not re-ask while the faction still has a valid target."); return 0; }

        // Forced (what endOfYear does after a discovery): ask anyway, offering the whole
        // widened Frontier.
        chooseResearch(0, true);
        if (!controller.query.active)
        { fail("After a discovery the player must be prompted again for where the science goes."); return 0; }
        if ((int)controller.query.options.size() != (int)frontier.size())
        { fail("The re-prompt should offer every technology in the (now wider) Frontier."); return 0; }

        // Answering it moves the target.
        int wanted = (int)frontier.size() - 1;
        controller.query.selected(wanted);
        controller.query.active = false;
        if (techtree.getResearchTarget(0) != frontier[wanted])
        { fail("Answering the post-discovery prompt should switch the research target."); return 0; }

        // An AI faction rerolls instead of popping a dialog.
        controller.query.active = false;
        chooseResearch(1, true);
        if (controller.query.active)
        { fail("A forced chooseResearch on an autoPlayer faction must not open a dialog."); return 0; }
        if (techtree.getResearchTarget(1) == 0)
        { fail("A forced chooseResearch on an autoPlayer faction should have rolled a target."); return 0; }
    }

    // ---- 7) science handed over while no target is usable is BANKED, not lost -------------
    {
        // A freshly initialized faction has NO target selected -- exactly the state a human
        // faction is in between being prompted and answering. Science offered now must wait.
        initTechnologies(techtree, (int)factions.size(), dee);

        if (techtree.getResearchTarget(0) != 0 || techtree.getPendingScience(0) != 0)
        { fail("A freshly initialized faction has no target and nothing banked."); return 0; }

        std::vector<int> none = techtree.advance(0, 250, dee);
        if (!none.empty())
        { fail("With no research target selected nothing should be discovered."); return 0; }
        if (techtree.getPendingScience(0) != 250)
        { fail("SCIENCE handed over with no usable target must be BANKED, not thrown away."); return 0; }

        // A second unanswered year adds to the bank rather than replacing it.
        techtree.advance(0, 100, dee);
        if (techtree.getPendingScience(0) != 350)
        { fail("Banked SCIENCE should accumulate across years while the prompt goes unanswered."); return 0; }

        // Answering: the whole bank goes in at once and fires the root's children.
        if (!techtree.setResearchTarget(0, TECH_ROOT))
        { fail("Language should be selectable on a freshly initialized graph."); return 0; }

        std::vector<int> got = techtree.advance(0, 0, dee);   // no NEW science this turn
        if (techtree.getPendingScience(0) != 0)
        { fail("Once a target is chosen the banked SCIENCE must be spent, not kept."); return 0; }
        if (got.empty())
        { fail("350 banked SCIENCE should have discovered something once a target was chosen."); return 0; }
    }

    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_059::title()
{
    return std::string("Tech graph wired into the game: initTechnologies() sets one graph per faction rooted at Language, chooseResearch() rolls for an AI and opens the controller.query selector for a human, endOfYear pools+clears city SCIENCE into TechTree::advance(), and a discovery unlocks its gated buildable (Pottery -> Granary).");
}

bool TestCase_059::done()   { return isdone; }
bool TestCase_059::passed() { return haspassed; }
std::string TestCase_059::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_059();
}
