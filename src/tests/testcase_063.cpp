//  TestCase_063.cpp
//  bunmei
//
//  Created by Claude on 20/09/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cmath>
#include <vector>
#include <unordered_map>

#include "../map.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../coordinator.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../codes.h"
#include "../dee.h"
#include "../technologies.h"

#include "testcase_063.h"

// @Task: README.md's per-dependency factors are RELATIVE, not absolute. A technology's
// incoming weights are now a CONVEX combination -- they keep the table's proportions but are
// rescaled to sum to TECH_DEFAULT_WEIGHT (TechGraph::normalizeWeights()) -- and
// TECH_BIAS_BASE drops to 1.0, so the bias is flat and depth prices nothing.
//
// The design consequence, and what this test pins down: what makes a technology hard is how
// many dependencies it has to SHARE its fan-in with, not how far from the root it sits.
//
// Two halves:
//   A. the rule itself, on a pinned synthetic graph shaped like README.md's Banking (three
//      parents at 0.9 / 0.9 / 1.0) plus a single-parent sibling. Node ids are 1..6, not
//      codes.h codes -- normalizeWeights() is graph machinery and must not care.
//   B. the same invariants on the graph the game actually ships (buildDefaultTechGraph), and
//      the measured cost of firing: equal share across all parents fires every technology at
//      the same per-parent level, whatever its fan-in.

extern std::unordered_map<int, std::string> tiles;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::vector<Faction*> factions;
extern Map map;
extern float mapzoom;

extern Coordinator coordinator;

#define TEST_MAPSIZE 1

TestCase_063::TestCase_063() {}
TestCase_063::~TestCase_063() {}

int TestCase_063::number() { return 63; }

void TestCase_063::init()
{
    // Pure graph logic (no map, no units), but the tester's game loop renders every tick and
    // indexes factions[coordinator.a_f_id], so a minimal world still has to exist.
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
    factions.push_back(faction);

    citynames[0] = std::queue<std::string>();

    mapzoom = 2;
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
}

int TestCase_063::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    // ================= A. the normalization rule, pinned ================================
    //
    //      R(root) --> P1 --,
    //          |             \
    //          +--> P2 -------+--> X      (0.9 / 0.9 / 1.0, like README.md's Banking)
    //          |             /
    //          `--> P3 -----'
    //                |
    //                `--> Y               (a single dependency, factor 0.5)
    {
        enum { R = 1, P1 = 2, P2 = 3, P3 = 4, X = 5, Y = 6 };

        TechGraph g;
        g.addTech(R, "R");
        g.addTech(P1, "P1");
        g.addTech(P2, "P2");
        g.addTech(P3, "P3");
        g.addTech(X, "X");
        g.addTech(Y, "Y");
        g.setRoot(R);

        g.addEdge(R, P1, 1.0f);
        g.addEdge(R, P2, 1.0f);
        g.addEdge(R, P3, 1.0f);
        g.addEdge(P1, X, 0.9f);
        g.addEdge(P2, X, 0.9f);
        g.addEdge(P3, X, 1.0f);
        g.addEdge(P3, Y, 0.5f);

        // Before normalizing, addEdge stores the RAW factor on the TECH_DEFAULT_WEIGHT scale.
        // That matters: a caller that wants the table's factors taken literally (testcase_058's
        // pinned scaffold graph does) simply never calls normalizeWeights().
        if (fabs(g.getWeight(P1, X) - 0.9f*TECH_DEFAULT_WEIGHT) > 0.0001f)
        { fail("addEdge() should store the raw factor * TECH_DEFAULT_WEIGHT until normalizeWeights() is called."); return 0; }

        g.normalizeWeights();

        // ---- convexity: every node's fan-in is worth exactly TECH_DEFAULT_WEIGHT ---------
        const int nodes[] = { P1, P2, P3, X, Y };
        for (int id : nodes)
        {
            float sum = 0.0f;
            for (const TechEdge& e : g.getTech(id)->inputs)
                sum += e.weight;
            if (fabs(sum - TECH_DEFAULT_WEIGHT) > 0.0001f)
            {
                char buf[160];
                snprintf(buf,sizeof(buf),"node %d's incoming weights sum to %.5f, not TECH_DEFAULT_WEIGHT (%.5f).",
                         id, sum, TECH_DEFAULT_WEIGHT);
                fail(buf); return 0;
            }
        }
        // The root has no parents and must be left alone rather than divided by zero.
        if (!g.getTech(R)->inputs.empty())
        { fail("The root has no parents -- normalizeWeights() must not invent any."); return 0; }

        // ---- the table's proportions survive --------------------------------------------
        // 0.9 / 0.9 / 1.0 of a total 2.8: P3 matters most, and by exactly the ratio the
        // factors declared. This is the property the whole change exists for.
        if (fabs(g.getWeight(P1, X) - (0.9f/2.8f)*TECH_DEFAULT_WEIGHT) > 0.0001f ||
            fabs(g.getWeight(P3, X) - (1.0f/2.8f)*TECH_DEFAULT_WEIGHT) > 0.0001f)
        { fail("X's weights should be each parent's SHARE of 2.8, scaled by TECH_DEFAULT_WEIGHT."); return 0; }
        if (fabs(g.getWeight(P1, X) - g.getWeight(P2, X)) > 0.0001f)
        { fail("P1 and P2 declare the same factor -- they must end with the same weight."); return 0; }
        if (g.getWeight(P3, X) <= g.getWeight(P1, X))
        { fail("P3 is X's (1.0) dependency against two (0.9)s: it must weigh more."); return 0; }
        if (fabs(g.getWeight(P3, X) / g.getWeight(P1, X) - (1.0f/0.9f)) > 0.001f)
        { fail("Normalization must preserve the RATIO the README factors declare (1.0 : 0.9)."); return 0; }

        // ---- a single dependency is the whole fan-in, whatever its factor ---------------
        // Y depends on P3 alone at (0.5). There is nothing to be a share OF, so it ends at
        // the full TECH_DEFAULT_WEIGHT -- a factor only ever means something against siblings.
        if (fabs(g.getWeight(P3, Y) - TECH_DEFAULT_WEIGHT) > 0.0001f)
        { fail("Y's only dependency must normalize to the whole TECH_DEFAULT_WEIGHT, ignoring its (0.5)."); return 0; }

        // ---- and it is idempotent --------------------------------------------------------
        g.normalizeWeights();
        if (fabs(g.getWeight(P3, X) - (1.0f/2.8f)*TECH_DEFAULT_WEIGHT) > 0.0001f)
        { fail("normalizeWeights() must be idempotent -- weights already convex should not move."); return 0; }

        // ---- what it costs to fire X ------------------------------------------------------
        g.computeBiases();
        const float LOGIT = logf(TECH_FIRING_THRESHOLD / (1.0f - TECH_FIRING_THRESHOLD));

        // Equal share across ALL of X's parents: the weights sum to TECH_DEFAULT_WEIGHT, so X
        // fires once each parent reaches (logit + bias) / TECH_DEFAULT_WEIGHT -- and so does
        // single-parent Y, at the same level. THAT is "every technology costs the same".
        const int expectedLevel = (int)ceilf((LOGIT + g.getBias(X)) / TECH_DEFAULT_WEIGHT);

        g.start();
        g.invest(R, 1000);                      // discover P1..P3, each still at 0 science
        g.step();
        if (!g.isDiscovered(P1) || !g.isDiscovered(P2) || !g.isDiscovered(P3))
        { fail("Setup: investing in the root should have discovered all three parents."); return 0; }
        if (g.isDiscovered(X) || g.isDiscovered(Y))
        { fail("Setup: X and Y are two layers out -- they must not fire off the root."); return 0; }

        int level = 0;
        while (level < 100000 && !g.isDiscovered(X))
        {
            g.invest(P1, 1); g.invest(P2, 1); g.invest(P3, 1);
            level++;
            g.step();
        }
        if (abs(level - expectedLevel) > 1)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"X fired with %d SCIENCE in each parent, the convex form says %d.",
                     level, expectedLevel);
            fail(buf); return 0;
        }
        // Y fired on the way, off P3 alone, at the very same per-parent level: three
        // dependencies or one, an evenly-fed technology costs what any other one does.
        if (!g.isDiscovered(Y))
        { fail("Y should have fired at the same per-parent level as X -- a flat bias makes fan-in width irrelevant when it is fed evenly."); return 0; }

        // Tunnelling through ONE parent is what costs more: P3 carries 1.0/2.8 of X, so
        // feeding only P3 needs 2.8x what feeding all three evenly did.
        TechGraph tunnel = g;
        tunnel.start();
        tunnel.invest(R, 1000);
        tunnel.step();
        int solo = 0;
        while (solo < 100000 && !tunnel.isDiscovered(X))
        {
            tunnel.invest(P3, 1);
            solo++;
            tunnel.step();
        }
        const int expectedSolo = (int)ceilf((LOGIT + tunnel.getBias(X)) / tunnel.getWeight(P3, X));
        if (abs(solo - expectedSolo) > 1)
        {
            char buf[200];
            snprintf(buf,sizeof(buf),"X fired with %d SCIENCE poured into P3 alone, the closed form says %d.",
                     solo, expectedSolo);
            fail(buf); return 0;
        }
        if (solo <= level)
        { fail("Concentrating on one parent must cost MORE than spreading across all of them."); return 0; }
    }

    // ================= B. the graph the game ships ======================================
    {
        TechGraph readme = buildDefaultTechGraph();

        // Convexity holds for all 46, and every weight is a share of the whole.
        for (int code = TECH_FIRST; code <= TECH_LAST; code++)
        {
            const Tech* t = readme.getTech(code);
            if (t == nullptr)
            { fail("buildDefaultTechGraph() is missing a codes.h technology."); return 0; }
            if (t->inputs.empty())
            {
                if (code != TECH_ROOT)
                { fail("Only the root may have no dependencies."); return 0; }
                continue;
            }

            float sum = 0.0f;
            for (const TechEdge& e : t->inputs)
            {
                if (e.weight <= 0.0f)
                { fail("A normalized weight must stay strictly positive."); return 0; }
                sum += e.weight;
            }
            if (fabs(sum - TECH_DEFAULT_WEIGHT) > 0.0001f)
            {
                char buf[180];
                snprintf(buf,sizeof(buf),"'%s' fan-in sums to %.5f, not TECH_DEFAULT_WEIGHT (%.5f).",
                         t->name.c_str(), sum, TECH_DEFAULT_WEIGHT);
                fail(buf); return 0;
            }
        }

        // README.md's own worked example: Banking is Currency (0.9), Code of Laws (0.9) and
        // Education (1.0). Education must matter most, and the three must divide the fan-in
        // between them in proportion -- 0.321 / 0.321 / 0.357.
        float wCurrency  = readme.getWeight(TECH_CURRENCY,     TECH_BANKING);
        float wCodeLaws  = readme.getWeight(TECH_CODE_OF_LAWS, TECH_BANKING);
        float wEducation = readme.getWeight(TECH_EDUCATION,    TECH_BANKING);
        if (fabs(wCurrency - wCodeLaws) > 0.0001f)
        { fail("Banking's Currency and Code of Laws are both (0.9): they must weigh the same."); return 0; }
        if (wEducation <= wCurrency)
        { fail("Banking's Education is its (1.0) dependency: it must weigh more than the (0.9)s."); return 0; }
        if (fabs(wCurrency  - (0.9f/2.8f)*TECH_DEFAULT_WEIGHT) > 0.0001f ||
            fabs(wEducation - (1.0f/2.8f)*TECH_DEFAULT_WEIGHT) > 0.0001f)
        { fail("Banking's weights should be 0.9/2.8 and 1.0/2.8 of TECH_DEFAULT_WEIGHT."); return 0; }
        if (fabs(wCurrency + wCodeLaws + wEducation - TECH_DEFAULT_WEIGHT) > 0.0001f)
        { fail("Banking's three weights must sum to TECH_DEFAULT_WEIGHT."); return 0; }

        // A wide fan-in dilutes every one of its dependencies: Astronomy has six parents, so
        // none of them can weigh what Hunting's single parent does.
        if (readme.getWeight(TECH_SHIP_BUILDING, TECH_ASTRONOMY) >= readme.getWeight(TECH_LANGUAGE, TECH_HUNTING))
        { fail("Astronomy shares its fan-in six ways -- no single parent may weigh as much as a sole dependency."); return 0; }

        // The flat bias, stated as the design intent rather than as BASE^depth: the deepest
        // technology in the table is no harder to fire than the shallowest.
        if (readme.getDepth(TECH_INDUSTRIALIZATION) <= readme.getDepth(TECH_HUNTING))
        { fail("Setup: Industrialization should still be far deeper than Hunting."); return 0; }
        if (fabs(readme.getBias(TECH_INDUSTRIALIZATION) - readme.getBias(TECH_HUNTING)) > 0.0001f)
        { fail("At TECH_BIAS_BASE 1.0 the bias must be flat: depth prices nothing any more."); return 0; }
    }

    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_063::title()
{
    return std::string("Convex tech weights (TechGraph::normalizeWeights) + flat bias: README.md's per-dependency factors are each dependency's SHARE of its technology's fan-in, summing to TECH_DEFAULT_WEIGHT, so difficulty comes from how many dependencies a technology has rather than from its depth.");
}

bool TestCase_063::done()   { return isdone; }
bool TestCase_063::passed() { return haspassed; }
std::string TestCase_063::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_063();
}
