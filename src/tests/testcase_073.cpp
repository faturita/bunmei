//  TestCase_073.cpp
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
#include "../units/Trireme.h"
#include "../City.h"
#include "../Faction.h"
#include "../coordinator.h"
#include "../commandorder.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../cityscreenui.h"

#include "testcase_073.h"

// @Task: the city screen's "Change" list used to set what a city builds from inside
// drawCityScreen() -- a RENDER pass popping and pushing city->productionQueue. Two things
// were wrong with that beyond the layering: the change only happened if the screen was drawn,
// and it could not be expressed by anything that is not a local UI.
//
// Now the click resolves the row to a BuildableId (buildable.h) and pushes
// Command::ChangeProductionOrder; the render only draws.
//
// By id rather than by row index, because an index means nothing without the exact list and
// scroll offset that were on screen when it was clicked; and by id rather than by name,
// because a name is display text that can change for presentation reasons while an id is the
// thing's identity. The handler looks the id up in the city's OWN buildable list, which is
// what makes accepting one from a caller safe. The registry behind those ids is checked here
// too -- the command is only as trustworthy as the id space it names.
//
// The Units box's activateUnit() call was migrated at the same time: it was the second call
// site of the direct call replaced by Command::ActivateUnitOrder in usercontrols.cpp, and it
// had been missed.

extern std::unordered_map<int, std::string> tiles;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Map map;
extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

extern bool changeIsActive;
extern int  selectionOffset;

#define TEST_MAPSIZE 1

// How many factories engine.cpp's registry holds. Pinned so a factory that fails to register
// -- a constructor that forgot its BuildableId, or two claiming the same one -- is visible as
// a count, since the registry itself drops such an entry rather than keeping a duplicate.
#define REGISTERED_BUILDABLES 24

TestCase_073::TestCase_073() {}
TestCase_073::~TestCase_073() {}

int TestCase_073::number() { return 73; }

void TestCase_073::init()
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
        }
    assignProductionRates(map);

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255; faction->green = 0; faction->blue = 0;
    faction->autoPlayer = false;
    factions.push_back(faction);
    citynames[0] = std::queue<std::string>();

    City *city = new City(&map, 0, getNextCityId(), 0, 0);
    city->setName("Kattegate");
    city->foundedyear = -4000;
    cities[city->id] = city;
    cityid = city->id;

    // A transport stationed in the city, for the Units box click.
    Trireme* t = new Trireme();
    t->id = getNextUnitId(); t->faction = 0;
    t->latitude = 0; t->longitude = 0;
    t->availablemoves = t->getUnitMoves();
    t->fortify();                            // so activating it has something to undo
    units[t->id] = t;
    transportid = t->id;

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = CONTROLLING_NONE;

    controller.cityid = city->id;
    controller.view = 2;
}

int TestCase_073::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };
    isdone = true;

    City* city = cities[cityid];
    coordinate cs = map.to_screen(city->latitude, city->longitude);

    // Fill the buildable list the way opening the Change box does.
    city->buildable.clear();
    populateCityBuildables(city);
    if (city->buildable.size() < 3)
    { fail("Setup: the city should have several buildables to choose between."); return 0; }

    const int first  = city->buildable[0]->getId();
    const int second = city->buildable[1]->getId();
    const std::string firstName = city->buildable[0]->name;

    changeIsActive  = true;
    selectionOffset = 0;

    // ---- 1) the click pushes, and changes nothing by itself -------------------------------
    {
        while (!city->productionQueue.empty()) city->productionQueue.pop();

        // Row 0 of the Change list: lat2 == 10, and any column that is not the arrows' (18).
        clickOnCityScreen(4, 6, 10, 8);

        if (!city->productionQueue.empty())
        { fail("The click changed the production queue directly -- it must only push a Command::ChangeProductionOrder."); return 0; }

        // A render pass must not apply it either: that is where this used to happen.
        drawCityScreen(cs.lat, cs.lon, city);
        if (!city->productionQueue.empty())
        { fail("drawCityScreen() changed the production queue -- a render pass must not mutate the city."); return 0; }

        processCommandOrders();

        if (city->productionQueue.empty())
        { fail("processCommandOrders() did not queue anything for Command::ChangeProductionOrder."); return 0; }
        if (city->productionQueue.front()->getId() != first)
        {
            char buf[220];
            snprintf(buf,sizeof(buf),"Clicking row 0 queued id %d, the first buildable is id %d.",
                     city->productionQueue.front()->getId(), first);
            fail(buf); return 0;
        }
    }

    // ---- 2) the row -> buildable mapping honours the scroll offset ------------------------
    // The offset is view state and stays on the client; what the command carries is the name
    // the offset resolved to.
    {
        changeIsActive  = true;
        selectionOffset = -1;               // list scrolled down by one

        clickOnCityScreen(4, 6, 10, 8);     // row 0 again -> index 1 now
        processCommandOrders();

        if (city->productionQueue.front()->getId() != second)
        {
            char buf[220];
            snprintf(buf,sizeof(buf),"With the list scrolled by one, row 0 queued id %d; it should be id %d.",
                     city->productionQueue.front()->getId(), second);
            fail(buf); return 0;
        }
        selectionOffset = 0;
    }

    // ---- 3) changing production REPLACES, it does not stack -------------------------------
    {
        if (city->productionQueue.size() != 1)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"The queue holds %d items after two changes; changing production replaces.",
                     (int)city->productionQueue.size());
            fail(buf); return 0;
        }
    }

    // ---- 4) the arrows scroll and must NOT select -----------------------------------------
    {
        changeIsActive = true;
        const int before = city->productionQueue.front()->getId();
        const int offsetBefore = selectionOffset;

        clickOnCityScreen(4, 6, 10, 18);    // the up arrow's own column
        processCommandOrders();

        if (city->productionQueue.front()->getId() != before)
        { fail("Clicking the scroll arrow changed the production queue -- it must only scroll."); return 0; }
        if (selectionOffset == offsetBefore)
        { fail("Clicking the scroll arrow did not scroll the list."); return 0; }
        selectionOffset = 0;
    }

    // ---- 5) server guardrails, pushed directly --------------------------------------------
    // A name the city cannot build, and a city that does not exist: neither may change
    // anything. This is the check that makes accepting a name from a caller safe.
    {
        const int before = city->productionQueue.front()->getId();

        auto pushChange = [&](int cid, int buildableid)
        {
            CommandOrder co;
            co.command = Command::ChangeProductionOrder;
            co.parameters.cityid = cid;
            co.parameters.selectedbuildableid = buildableid;
            coordinator.push(co);
            processCommandOrders();
        };

        // A real, registered buildable that this city cannot currently build: the city's own
        // list is the authority, not the registry.
        bool cityHasObservatory = false;
        for (BuildableFactory* bf : city->buildable)
            if (bf->getId() == BUILDABLE_OBSERVATORY) cityHasObservatory = true;
        if (cityHasObservatory)
        { fail("Setup: this city can build an Observatory, so it cannot stand in for one it cannot build."); return 0; }

        pushChange(cityid, BUILDABLE_OBSERVATORY);
        if (city->productionQueue.front()->getId() != before)
        { fail("A registered buildable the city cannot currently build was queued anyway."); return 0; }

        pushChange(cityid, BUILDABLE_NONE);       // the "nothing" id
        if (city->productionQueue.front()->getId() != before)
        { fail("BUILDABLE_NONE matched something."); return 0; }

        pushChange(cityid, 31337);                // not a BuildableId at all
        if (city->productionQueue.front()->getId() != before)
        { fail("An id that is not a buildable at all matched something."); return 0; }

        pushChange(99999, first);                 // no such city
        if (city->productionQueue.front()->getId() != before)
        { fail("An order naming a city that does not exist changed a real city."); return 0; }
        if (cities.find(99999) != cities.end())
        { fail("Looking up a missing city id inserted it -- use find(), never cities[id]."); return 0; }
    }

    // ---- 6) the registry the ids come from ------------------------------------------------
    // The command is only as trustworthy as its id space: if two factories shared an id, or
    // one had none, a perfectly valid-looking order would queue the wrong thing.
    {
        std::vector<BuildableFactory*> all = allBuildableFactories();

        // An EXACT count, not a lower bound. The registry refuses a factory whose id is
        // missing or already claimed -- which is the right thing to do, but it means a
        // duplicate id shows up only as an entry that quietly never arrived. Pinning the
        // count is what makes that visible. Bump it when a buildable is added.
        if ((int)all.size() != REGISTERED_BUILDABLES)
        {
            char buf[220];
            snprintf(buf,sizeof(buf),"The buildable registry holds %d factories, expected %d -- one was dropped "
                                     "(a missing or duplicated BuildableId) or a new one needs counting here.",
                     (int)all.size(), REGISTERED_BUILDABLES);
            fail(buf); return 0;
        }

        std::vector<int> seen;
        for (BuildableFactory* bf : all)
        {
            if (bf->getId() == BUILDABLE_NONE)
            {
                char buf[200];
                snprintf(buf,sizeof(buf),"Registered factory '%s' has no BuildableId -- its constructor must set one.", bf->name);
                fail(buf); return 0;
            }
            for (int other : seen)
                if (other == bf->getId())
                {
                    char buf[200];
                    snprintf(buf,sizeof(buf),"BuildableId %d is claimed by more than one factory (at '%s').", bf->getId(), bf->name);
                    fail(buf); return 0;
                }
            seen.push_back(bf->getId());

            // The lookup the handler relies on has to return the SAME instance: city->buildable
            // holds pointers into the registry, so a copy would compare unequal.
            if (buildableFactoryById(bf->getId()) != bf)
            {
                char buf[200];
                snprintf(buf,sizeof(buf),"buildableFactoryById(%d) did not return the registered '%s'.", bf->getId(), bf->name);
                fail(buf); return 0;
            }
        }

        if (buildableFactoryById(BUILDABLE_NONE) != nullptr)
        { fail("BUILDABLE_NONE must not resolve to a factory."); return 0; }
        if (buildableFactoryById(31337) != nullptr)
        { fail("An unknown id must resolve to nullptr, not to something arbitrary."); return 0; }

        // And the ids really are the stable numbers the enum promises, not positions in a
        // list that could be reordered.
        BuildableFactory* warrior = buildableFactoryById(BUILDABLE_WARRIOR);
        if (warrior == nullptr || strcmp(warrior->name, "Warrior") != 0)
        { fail("BUILDABLE_WARRIOR does not resolve to the Warrior factory."); return 0; }
    }

    // ---- 7) the Units box activates through a command too ---------------------------------
    {
        changeIsActive = false;
        coordinator.a_u_id = CONTROLLING_NONE;
        units[transportid]->fortify();

        // Row 5 is the first stationed-unit row; lon 0 is inside the box, away from the
        // cargo-slot columns.
        clickOnCityScreen(5, 0, 0, 0);

        if (coordinator.a_u_id == transportid)
        { fail("The Units box activated the unit directly -- it must push a Command::ActivateUnitOrder."); return 0; }
        if (!units[transportid]->isFortified())
        { fail("The Units box packed the unit up directly, without going through the command."); return 0; }

        processCommandOrders();

        if (coordinator.a_u_id != transportid)
        { fail("processCommandOrders() did not activate the unit clicked in the Units box."); return 0; }
        if (units[transportid]->isFortified())
        { fail("Activating the stationed unit should have packed it up."); return 0; }
    }

    haspassed = true;
    return 0;
}

std::string TestCase_073::title()
{
    return std::string("City screen 'Change' migrated to Command::ChangeProductionOrder: the click resolves the row (scroll offset included) to a buildable NAME and pushes, drawCityScreen() no longer mutates the production queue, and the handler refuses a name the city cannot build. The Units box's activateUnit() call -- the one site missed when the map click was migrated -- now pushes Command::ActivateUnitOrder too.");
}

bool TestCase_073::done()   { return isdone; }
bool TestCase_073::passed() { return haspassed; }
std::string TestCase_073::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_073();
}
