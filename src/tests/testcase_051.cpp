//  TestCase_051.cpp
//  bunmei
//
//  Created by Claude on 05/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <algorithm>

#include "../map.h"
#include "../units/Unit.h"
#include "../units/Ship.h"
#include "../units/Trireme.h"
#include "../units/Galleon.h"
#include "../units/Galley.h"
#include "../units/Warrior.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"

#include "testcase_051.h"

// @Task: Trireme and Galleon now both derive from a shared Ship interface (Unit + Transport).
// All the naval-handling code (engine.cpp: findNavalUnit / moveOntoNavalUnit / dockInCity /
// land) was changed to work through Ship* instead of Trireme*, so a Galleon -- and any future
// ship -- goes through exactly the same paths a Trireme does. This checks that at the seam:
//   - findNavalUnit() returns BOTH a Trireme and a Galleon (dynamic_cast<Ship*>), but NOT a
//     Galley (an OCEANTYPE Unit that is not a Ship/Transport).
//   - moveOntoNavalUnit() boards a passenger onto a Galleon just as onto a Trireme.

extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Tiles tiles;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

#define TEST_MAPSIZE 1

TestCase_051::TestCase_051() {}
TestCase_051::~TestCase_051() {}

int TestCase_051::number()
{
    return 51;
}

void TestCase_051::init()
{
    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon) = mapcell(OCEAN);
    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
            map.set(lat,lon).setVisible(0);

    Faction *f = new Faction();
    f->id = 0; strcpy(f->name,"Vikings");
    f->red = 255; f->green = 0; f->blue = 0;
    f->autoPlayer = false;
    factions.push_back(f);

    citynames[0] = std::queue<std::string>();

    Galleon *g = new Galleon();
    g->id = getNextUnitId(); g->faction = 0;
    g->latitude = 5; g->longitude = 5;
    g->availablemoves = g->getUnitMoves();
    units[g->id] = g; galleonid = g->id;

    Trireme *t = new Trireme();
    t->id = getNextUnitId(); t->faction = 0;
    t->latitude = 5; t->longitude = 3;
    t->availablemoves = t->getUnitMoves();
    units[t->id] = t; triremeid = t->id;

    Galley *y = new Galley();
    y->id = getNextUnitId(); y->faction = 0;
    y->latitude = 5; y->longitude = 7;
    y->availablemoves = y->getUnitMoves();
    units[y->id] = y; galleyid = y->id;

    Warrior *w = new Warrior();
    w->id = getNextUnitId(); w->faction = 0;
    w->latitude = 5; w->longitude = 4;
    w->availablemoves = w->getUnitMoves();
    units[w->id] = w; warriorid = w->id;

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = warriorid;
}

int TestCase_051::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    // 1) findNavalUnit() finds the Galleon at (5,5) via dynamic_cast<Ship*>.
    Ship* atGalleon = findNavalUnit(5,5);
    if (atGalleon == nullptr || dynamic_cast<Unit*>(atGalleon) != units[galleonid])
    {
        fail("findNavalUnit did not return the Galleon at (5,5) as a Ship*.");
        return 0;
    }

    // 2) It also still finds the Trireme (unchanged behaviour).
    Ship* atTrireme = findNavalUnit(5,3);
    if (atTrireme == nullptr || dynamic_cast<Unit*>(atTrireme) != units[triremeid])
    {
        fail("findNavalUnit no longer returns the Trireme at (5,3).");
        return 0;
    }

    // 3) A Galley sits at (5,7): it is OCEANTYPE but NOT a Ship/Transport, so findNavalUnit
    //    must return nullptr there (nothing to board).
    if (findNavalUnit(5,7) != nullptr)
    {
        fail("findNavalUnit returned a non-null Ship* for a Galley (which is not a Ship).");
        return 0;
    }

    // 4) moveOntoNavalUnit() boards the Warrior onto the Galleon, exactly the Trireme path.
    Unit* warrior = units[warriorid];
    bool boarded = moveOntoNavalUnit(warrior, findNavalUnit(5,5), 5, 5);
    Ship* galleon = findNavalUnit(5,5);
    if (!boarded || galleon->manifest() != 1
        || warrior->latitude != 5 || warrior->longitude != 5 || !warrior->isSentry())
    {
        char buf[200];
        snprintf(buf,sizeof(buf),"moveOntoNavalUnit onto a Galleon failed (boarded=%d, manifest=%d, warrior at (%d,%d) sentry=%d).",
                 boarded, galleon->manifest(), warrior->latitude, warrior->longitude, warrior->isSentry());
        fail(buf);
        return 0;
    }

    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_051::title()
{
    return std::string("Ship interface: findNavalUnit / moveOntoNavalUnit work on a Galleon exactly as on a Trireme (via Ship*), and skip a non-Ship Galley.");
}

bool TestCase_051::done()   { return isdone; }
bool TestCase_051::passed() { return haspassed; }
std::string TestCase_051::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_051();
}
