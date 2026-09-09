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
// (3) is measured, not assumed. Weights and biases are DATA (README.md's per-dependency
// factors scaled by TECH_DEFAULT_WEIGHT, and bias = TECH_BIAS_BASE ^ depth), so a discovery
// costs exactly ceil((logit(threshold) + bias(child)) / weight(parent,child)). The test steps
// the game's own graph and compares the measured cost against that closed form, then checks
// the two things the knobs are supposed to buy: a lighter dependency costs more, and each
// extra hop of depth costs more.
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

    // ---- 3) knob calibration -------------------------------------------------------------
    // Weights and biases are DATA now (README.md's factors, and TECH_BIAS_BASE^depth), so the
    // cost of a discovery is exact rather than an average over random draws:
    //     science = ceil( (logit(TECH_FIRING_THRESHOLD) + bias(child)) / weight(parent,child) )
    // This measures it by actually stepping the game's own graph and compares against that
    // closed form, so a change to either knob shows up here immediately.
    {
        const float LOGIT = logf(TECH_FIRING_THRESHOLD / (1.0f - TECH_FIRING_THRESHOLD));

        // Cost of firing `child` by pouring SCIENCE into `parent`, measured one point at a time.
        auto measure = [&](int parent, int child)->int
        {
            TechGraph g = buildDefaultTechGraph();
            g.start();
            // Only a Frontier technology can be invested in, so a non-root parent has to be
            // researched for real first (off the root). That leaves it discovered with 0
            // science of its own, so the count below starts clean.
            if (parent != TECH_ROOT)
            {
                int guard = 0;
                while (!g.isDiscovered(parent) && guard++ < 200000)
                {
                    if (!g.invest(TECH_ROOT, 1))
                        return -1;
                    g.step();
                }
                if (!g.isDiscovered(parent))
                    return -1;
            }
            int spent = 0;
            while (spent < 200000)
            {
                if (!g.invest(parent, 1))
                    return -1;                       // parent not investable
                spent++;
                std::vector<int> fired = g.step();
                for (int id : fired) if (id == child) return spent;
            }
            return -1;
        };

        auto expected = [&](int parent, int child)->int
        {
            TechGraph g = buildDefaultTechGraph();
            float w = g.getWeight(parent, child);
            float b = g.getBias(child);
            return (int)ceilf((LOGIT + b) / w);
        };

        // A depth-1 technology straight off the root, at the table's default (1.0) weight.
        int gotHunting = measure(TECH_ROOT, TECH_HUNTING);
        int expHunting = expected(TECH_ROOT, TECH_HUNTING);
        if (gotHunting < 0 || abs(gotHunting - expHunting) > 1)
        {
            char buf[220];
            snprintf(buf,sizeof(buf),"Hunting cost %d SCIENCE off Language, the closed form says %d.",
                     gotHunting, expHunting);
            fail(buf); return 0;
        }
        if (gotHunting < SCIENCE_BAND_LOW || gotHunting > SCIENCE_BAND_HIGH)
        {
            char buf[220];
            snprintf(buf,sizeof(buf),
                     "The first discovery costs %d SCIENCE; the knobs are tuned for about %d (band %d..%d).",
                     gotHunting, TARGET_SCIENCE_PER_DISCOVERY, SCIENCE_BAND_LOW, SCIENCE_BAND_HIGH);
            fail(buf); return 0;
        }

        // A lighter dependency costs proportionally more: Language -> Masonry is (0.9).
        int gotMasonry = measure(TECH_ROOT, TECH_MASONRY);
        if (gotMasonry <= gotHunting)
        { fail("Masonry hangs off a (0.9) dependency -- it must cost MORE than a (1.0) one at the same depth."); return 0; }

        // Depth is what makes late technologies expensive: bias = TECH_BIAS_BASE ^ depth, so
        // each extra hop multiplies the cost by roughly TECH_BIAS_BASE.
        int gotWriting = measure(TECH_ALPHABET, TECH_WRITING);      // depth 2, factor 1.0
        int expWriting = expected(TECH_ALPHABET, TECH_WRITING);
        if (gotWriting < 0 || abs(gotWriting - expWriting) > 1)
        {
            char buf[220];
            snprintf(buf,sizeof(buf),"Writing cost %d SCIENCE off Alphabet, the closed form says %d.",
                     gotWriting, expWriting);
            fail(buf); return 0;
        }
        if (gotWriting <= gotHunting)
        { fail("A depth-2 technology must cost more than a depth-1 one."); return 0; }

        // And the deepest technology in the table is dramatically more expensive still.
        TechGraph probe = buildDefaultTechGraph();
        if (probe.getBias(TECH_INDUSTRIALIZATION) <= probe.getBias(TECH_HUNTING) * 100.0f)
        { fail("Industrialization (depth 12) should carry a vastly larger bias than a depth-1 technology."); return 0; }

        // Sanity on the knobs themselves.
        if (TECH_DEFAULT_WEIGHT <= 0.0f || TECH_BIAS_BASE <= 1.0f)
        { fail("TECH_DEFAULT_WEIGHT must be positive and TECH_BIAS_BASE > 1, or depth would not cost anything."); return 0; }
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
