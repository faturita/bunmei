//  TestCase_060.cpp
//  bunmei
//
//  Created by Claude on 08/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cmath>
#include <vector>
#include <string>

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

#include "testcase_060.h"

// @Task: (1) a /fundamental teletype command that pushes a CommandOrder setting the calling
// faction's TRADE conversion rates, (2) a 0.5/0.5/0/0 default so SCIENCE is actually produced,
// (3) tech knobs scaled down 10x for roughly one discovery per 80 SCIENCE.
//
// (1) is driven end to end through the REAL input path: handleKeypress() one character at a
// time, Enter, then processCommandOrders() -- so the parser, the CommandOrder and the engine
// handler are all exercised, not just the handler.
// (3) is measured, not assumed: the firing rule is
//       science >= (logit(TECH_FIRING_THRESHOLD) + bias) / weight
// so the cost is dominated by 1/weight. Two figures are measured over many randomized graphs:
// the cost of ONE edge (~80, what the knobs tune) and the cost of the first discovery off the
// real root (much lower -- Language feeds 13 children and the luckiest edge wins the race).
//
// (2) cannot be checked here: the default rates live in gamekernel.cpp's FACTION_DEFINITIONS
// (and simulate.cpp's own initFactions), neither of which is linked into the testcase build --
// they are verified with a live ./bunmei run instead (see PROJECT.md).

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

// How many SCIENCE a discovery is meant to cost, and how far the measured average may drift
// before this test complains (the knobs are random per graph, so it is an average).
#define TARGET_SCIENCE_PER_DISCOVERY 80
#define SCIENCE_BAND_LOW             50
#define SCIENCE_BAND_HIGH           110

TestCase_060::TestCase_060() {}
TestCase_060::~TestCase_060() {}

int TestCase_060::number() { return 60; }

void TestCase_060::init()
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

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255; faction->green = 0; faction->blue = 0;
    faction->autoPlayer = false;
    faction->rates[0] = 0.5f; faction->rates[1] = 0.5f;
    faction->rates[2] = 0.0f; faction->rates[3] = 0.0f;
    factions.push_back(faction);

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
}

// Types a teletype line the way a player does -- one keypress per character, then Enter.
static void typeTeletype(const char* line)
{
    controller.teletype = true;
    controller.str.clear();
    for (const char* p = line; *p; p++)
        handleKeypress((unsigned char)*p, 0, 0);
    handleKeypress(13, 0, 0);       // Enter: parses and pushes the CommandOrder
}

int TestCase_060::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    // ---- 1) /fundamental, through the real keyboard -> CommandOrder -> engine path -------
    {
        Faction* f = factions[0];
        f->rates[0] = 1.0f; f->rates[1] = 0.0f; f->rates[2] = 0.0f; f->rates[3] = 0.0f;

        typeTeletype("/fundamental 0.25,0.75,0.1,0.2");

        // The command must go through the queue, not mutate the faction inline.
        if (f->rates[1] != 0.0f)
        { fail("/fundamental must push a CommandOrder, not change the faction before it is processed."); return 0; }

        processCommandOrders();

        if (fabs(f->rates[0]-0.25f) > 0.0001f || fabs(f->rates[1]-0.75f) > 0.0001f ||
            fabs(f->rates[2]-0.10f) > 0.0001f || fabs(f->rates[3]-0.20f) > 0.0001f)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"/fundamental did not set the rates (got %.2f,%.2f,%.2f,%.2f, expected 0.25,0.75,0.10,0.20).",
                     f->rates[0], f->rates[1], f->rates[2], f->rates[3]);
            fail(buf); return 0;
        }

        // It addresses the CALLING faction (coordinator.a_f_id at push time).
        Faction* other = new Faction();
        other->id = 1;
        strcpy(other->name,"Romans");
        other->autoPlayer = true;
        other->rates[0] = 1.0f; other->rates[1] = 0.0f; other->rates[2] = 0.0f; other->rates[3] = 0.0f;
        factions.push_back(other);

        coordinator.a_f_id = 1;
        typeTeletype("/fundamental 0.3,0.3,0.4,0.0");
        processCommandOrders();
        coordinator.a_f_id = 0;

        if (fabs(other->rates[1]-0.3f) > 0.0001f)
        { fail("/fundamental should address the faction that issued it."); return 0; }
        if (fabs(f->rates[1]-0.75f) > 0.0001f)
        { fail("/fundamental must not touch any other faction's rates."); return 0; }

        // A malformed list is refused outright (no command pushed, nothing changed).
        typeTeletype("/fundamental 0.9,0.1");
        processCommandOrders();
        if (fabs(f->rates[1]-0.75f) > 0.0001f || fabs(other->rates[1]-0.3f) > 0.0001f)
        { fail("/fundamental with fewer than four values must be rejected, leaving the rates alone."); return 0; }
    }

    // ---- 3) knob calibration: measure the real SCIENCE cost of a discovery ---------------
    // Two different numbers, both worth pinning:
    //   A. ONE edge: the cost the firing rule actually defines,
    //        science >= (logit(TECH_FIRING_THRESHOLD) + bias) / weight
    //      averaged over the random weight/bias ranges. This is what "the knobs" tune.
    //   B. The FIRST discovery off the real graph's root. Language feeds 13 children, and
    //      step() fires as soon as ANY of them crosses the threshold -- so this is the cost
    //      of the LUCKIEST of 13 edges and comes out far below A. It is what a player
    //      actually experiences, so it is measured too.
    {
        const int SAMPLES = 60;
        const int GIVE_UP = 8000;              // far beyond any sane cost; guards a broken knob

        // --- A: a bare two-node graph, one edge, no "best of N" effect ---
        TechGraph single;
        single.addTech(1, "Root");
        single.addTech(2, "Leaf");
        single.setRoot(1);
        single.addEdge(1, 2);

        long totalEdge = 0;
        for (int s=0;s<SAMPLES;s++)
        {
            TechGraph g = single;
            g.randomize();
            g.start();

            int spent = 0;
            while (spent < GIVE_UP)
            {
                g.invest(1, 1);
                spent++;
                if (!g.step().empty())
                    break;
            }
            if (spent >= GIVE_UP)
            { fail("A single edge needed more than 8000 SCIENCE to fire -- the tech knobs are broken."); return 0; }
            totalEdge += spent;
        }
        int edgeCost = (int)(totalEdge / SAMPLES);

        if (edgeCost < SCIENCE_BAND_LOW || edgeCost > SCIENCE_BAND_HIGH)
        {
            char buf[220];
            snprintf(buf,sizeof(buf),
                     "One edge costs %d SCIENCE on average; the knobs are tuned for about %d (band %d..%d).",
                     edgeCost, TARGET_SCIENCE_PER_DISCOVERY, SCIENCE_BAND_LOW, SCIENCE_BAND_HIGH);
            fail(buf); return 0;
        }

        // --- B: the game's own graph, root -> first discovery ---
        TechGraph prototype = buildDefaultTechGraph();
        long totalFirst = 0;
        for (int s=0;s<SAMPLES;s++)
        {
            TechGraph g = prototype;
            g.randomize();
            g.start();

            int spent = 0;
            while (spent < GIVE_UP)
            {
                g.invest(TECH_ROOT, 1);
                spent++;
                if (!g.step().empty())
                    break;
            }
            if (spent >= GIVE_UP)
            { fail("The first discovery cost more than 8000 SCIENCE -- the tech knobs are broken."); return 0; }
            totalFirst += spent;
        }
        int firstCost = (int)(totalFirst / SAMPLES);

        // Language has 13 children, so this lands well under the per-edge cost. The band is
        // wide on purpose -- it is here to catch a knob change that breaks the scale, not to
        // pin a balance decision.
        if (firstCost < 10 || firstCost > edgeCost)
        {
            char buf[240];
            snprintf(buf,sizeof(buf),
                     "The first discovery off Language cost %d SCIENCE (one edge costs %d); expected it between 10 and the per-edge cost.",
                     firstCost, edgeCost);
            fail(buf); return 0;
        }

        // Sanity on the knobs themselves: this is the 10x-smaller range, not the old one.
        if (TECH_WEIGHT_MAX > 0.1f)
        { fail("TECH_WEIGHT_MAX is back at the old (10x larger) scale -- discoveries would be ~7 SCIENCE."); return 0; }
    }

    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_060::title()
{
    return std::string("/fundamental teletype command pushes a SetFundamentalRatesOrder for the calling faction (driven through handleKeypress), and the 10x-smaller tech weight knobs cost about 80 SCIENCE per discovery.");
}

bool TestCase_060::done()   { return isdone; }
bool TestCase_060::passed() { return haspassed; }
std::string TestCase_060::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_060();
}
