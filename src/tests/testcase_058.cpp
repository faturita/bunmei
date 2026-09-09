//  TestCase_058.cpp
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
#include <unordered_map>
#include <unordered_set>

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

#include "testcase_058.h"

// @Task: tech tree scaffold (technologies.{h,cpp}) -- a graph, not a tree, wired like an MLP.
// SCIENCE is invested into technologies a faction ALREADY knows (the "Frontier"); that value
// propagates forward through weighted edges, and a candidate ("Next") fires -- is discovered
// -- once sigmoid(SUM(parent.science * w) - bias) crosses TECH_FIRING_THRESHOLD. Discovery
// registers the technology's codes.h dependency code at faction scope in the DEE.
//
// Two halves, per the task ("the structure of the graph network is in README.md -- use it for
// testing, not for implementing now. Test the scaffold separately."):
//   A. the scaffold's mechanics, on a tiny 4-node graph with PINNED weights/biases so every
//      firing decision is exact and nothing depends on the random initialization. Its node
//      ids are deliberately NOT codes.h codes -- the scaffold must not assume they are.
//   B. the real README.md:307 table (46 technologies), whose codes now live in codes.h, built
//      here as TEST DATA and driven through TechTree::advance() for two factions to check the
//      graph shape, the Frontier/Next bookkeeping at scale, the DEE bridge, and per-faction
//      independence.

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

// README.md:307's table now lives in technologies.cpp (buildDefaultTechGraph) because the
// game needs it -- this test drives THAT graph, so the two cannot drift apart. The numbers
// below are the README table's own shape.
#define README_TECH_COUNT       46
#define README_DEPENDENCY_COUNT 118
#define LANGUAGE_CHILD_COUNT    13

TestCase_058::TestCase_058() {}
TestCase_058::~TestCase_058() {}

int TestCase_058::number() { return 58; }

void TestCase_058::init()
{
    // The scaffold is pure logic (no map, no units), but the tester's game loop renders every
    // tick and indexes factions[coordinator.a_f_id], so a minimal world still has to exist.
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

static bool contains(const std::vector<int>& v, int id)
{
    for (int x : v) if (x == id) return true;
    return false;
}

int TestCase_058::check(int year)
{
    ticks++;
    if (isdone) return 0;
    if (ticks < 3) return 0;

    auto fail = [&](const std::string& m){ isdone = true; haspassed = false; message = m; };

    // ================= A. scaffold mechanics, pinned weights =============================
    //
    //      A(root) --> B --> D
    //          \           /
    //           `--> C ---'
    //
    // Node ids here are 1..4, NOT codes.h codes: the scaffold must work for any graph, and
    // node B carries a depCode that differs from its id to keep the two roles honest.
    {
        enum { A = 1, B = 2, C = 3, D = 4 };

        TechGraph g;
        g.addTech(A, "A");
        g.addTech(B, "B", TECH_ALPHABET);   // id 2, depCode 0x0d -- deliberately different
        g.addTech(C, "C");
        g.addTech(D, "D");
        g.setRoot(A);
        g.addEdge(A,B); g.addEdge(A,C); g.addEdge(B,D); g.addEdge(C,D);

        if (g.size() != 4)
        { fail("TechGraph::size() should be 4 after adding 4 techs."); return 0; }
        if (g.getTech(B)->depCode != TECH_ALPHABET || g.getTech(A)->depCode != TECH_NO_DEP_CODE)
        { fail("A node's depCode is independent of its graph id (and defaults to TECH_NO_DEP_CODE)."); return 0; }

        // addEdge with no factor stores the table's default weight, on the global scale.
        if (fabs(g.getWeight(A,B) - TECH_DEFAULT_WEIGHT) > 0.0001f)
        { fail("addEdge() with no factor should store exactly TECH_DEFAULT_WEIGHT."); return 0; }
        // A factor is relative to that default.
        g.addEdge(A,C);                      // duplicate, ignored -- C was already wired below
        if (fabs(g.getWeight(A,C) - TECH_DEFAULT_WEIGHT) > 0.0001f)
        { fail("A re-added edge must keep its original weight."); return 0; }

        // Depths and biases are only assigned by computeBiases(); before that a node's bias is 0.
        if (g.getBias(D) != 0.0f)
        { fail("A node's bias should be 0 until computeBiases() runs."); return 0; }
        g.computeBiases();
        if (g.getDepth(A) != 0 || g.getDepth(B) != 1 || g.getDepth(C) != 1 || g.getDepth(D) != 2)
        { fail("computeBiases() should set depth = longest path in hops from the root (A0 B1 C1 D2)."); return 0; }
        if (fabs(g.getBias(A) - 1.0f) > 0.0001f ||
            fabs(g.getBias(D) - TECH_BIAS_BASE*TECH_BIAS_BASE) > 0.0001f)
        { fail("computeBiases() should set bias = TECH_BIAS_BASE ^ depth."); return 0; }

        // Pin everything so no assertion below depends on the random initialization.
        g.setWeight(A,B, 0.5f);  g.setBias(B, 0.1f);   // fires on ~10 science in A
        g.setWeight(A,C, 0.5f);  g.setBias(C, 0.9f);
        g.setWeight(B,D, 0.5f);  g.setWeight(C,D, 0.5f); g.setBias(D, 0.1f);

        g.start();

        if (!g.isDiscovered(A) || g.isDiscovered(B) || g.isDiscovered(C) || g.isDiscovered(D))
        { fail("After start() only the root should be discovered."); return 0; }
        if (g.getFrontier().size() != 1 || g.getFrontier().count(A) != 1)
        { fail("After start() the Frontier should be exactly the root."); return 0; }
        if (g.getNext().size() != 2 || g.getNext().count(B) != 1 || g.getNext().count(C) != 1)
        { fail("After start() Next should be the root's two children."); return 0; }

        // You can only research what you already know.
        if (g.invest(B, 10))
        { fail("invest() into a non-Frontier tech must be rejected."); return 0; }
        if (g.invest(A, 0) || g.invest(A, -5))
        { fail("invest() with a non-positive amount must be rejected."); return 0; }

        // Nothing invested yet -> sigmoid(0 - bias) is well under the threshold.
        if (g.activation(B) >= TECH_FIRING_THRESHOLD)
        { fail("With no science invested nothing should be anywhere near firing."); return 0; }

        if (!g.invest(A, 10))
        { fail("invest() into the root (a Frontier tech) should be accepted."); return 0; }

        // B: sigmoid(10*0.5 - 0.1) = sigmoid(4.9) ~ 0.9926  -> fires
        // C: sigmoid(10*0.5 - 0.9) = sigmoid(4.1) ~ 0.9836  -> also fires; drop C's weight so
        //    this step promotes exactly one node and the Frontier/Next maths stays legible.
        g.setWeight(A,C, 0.0f);   // sigmoid(0 - 0.9) = 0.289
        if (fabs(g.activation(B) - techSigmoid(4.9f)) > 0.0001f)
        { fail("activation() does not match sigmoid(science*w - bias) for a single parent."); return 0; }

        std::vector<int> fired = g.step();
        if (fired.size() != 1 || fired[0] != B)
        { fail("step() should have discovered exactly B."); return 0; }
        if (!g.isDiscovered(B) || g.isDiscovered(C) || g.isDiscovered(D))
        { fail("After step() only B should have been added to the discovered set."); return 0; }
        // A still has an undiscovered child (C) so it stays; B joins the Frontier.
        if (g.getFrontier().count(A) != 1 || g.getFrontier().count(B) != 1 || g.getFrontier().size() != 2)
        { fail("After B fires the Frontier should be {A, B}."); return 0; }
        // D now fans out of B, C still fans out of A.
        if (g.getNext().size() != 2 || g.getNext().count(C) != 1 || g.getNext().count(D) != 1)
        { fail("After B fires Next should be {C, D}."); return 0; }

        // A multi-parent node sums its parents: D fires off B alone even though its other
        // parent C is still undiscovered (an undiscovered parent just contributes 0).
        if (!g.invest(B, 20))
        { fail("invest() into B should be accepted now that B is in the Frontier."); return 0; }
        float expectedD = techSigmoid(20*0.5f + 0*0.5f - 0.1f);
        if (fabs(g.activation(D) - expectedD) > 0.0001f)
        { fail("activation() does not sum over several parents as sigmoid(SUM(science*w) - bias)."); return 0; }

        fired = g.step();
        if (!contains(fired, D) || !g.isDiscovered(D))
        { fail("D should have fired from B's science alone (multi-parent sum, C contributing 0)."); return 0; }
        if (g.isDiscovered(C))
        { fail("C must NOT have been discovered -- its only edge weight was pinned to 0."); return 0; }

        // B's only child is discovered -> B leaves the Frontier. D has no children at all ->
        // it never stays in the Frontier. A still has C to reach.
        if (g.getFrontier().count(B) != 0)
        { fail("B should have been pruned from the Frontier (all of its children are discovered)."); return 0; }
        if (g.getFrontier().count(D) != 0)
        { fail("D is a leaf -- it should not remain in the Frontier."); return 0; }
        if (g.getFrontier().size() != 1 || g.getFrontier().count(A) != 1)
        { fail("The Frontier should be back to just {A} (only C is left to discover)."); return 0; }
        if (g.getNext().size() != 1 || g.getNext().count(C) != 1)
        { fail("Next should be exactly {C}."); return 0; }
        if (g.invest(B, 10))
        { fail("invest() into a tech that was pruned out of the Frontier must be rejected."); return 0; }

        // start() rewinds progress but keeps the pinned weights.
        g.start();
        if (g.isDiscovered(B) || g.getTech(A)->science != 0)
        { fail("start() should reset discovery and every node's accumulated science."); return 0; }
        if (fabs(g.getWeight(A,B) - 0.5f) > 0.0001f)
        { fail("start() must not touch weights."); return 0; }
    }

    // ================= B. the README.md:307 graph, end to end ============================
    {
        // codes.h carries the whole README table, so its own bookkeeping must line up.
        if (TECH_COUNT != README_TECH_COUNT)
        {
            char buf[160];
            snprintf(buf,sizeof(buf),"codes.h says TECH_COUNT=%d but README.md lists %d technologies.",
                     (int)TECH_COUNT, README_TECH_COUNT);
            fail(buf); return 0;
        }
        if (TECH_ROOT != TECH_LANGUAGE)
        { fail("codes.h TECH_ROOT should be TECH_LANGUAGE."); return 0; }

        // The graph the GAME ships (technologies.cpp), not a copy of it.
        TechGraph readme = buildDefaultTechGraph();

        if (readme.size() != README_TECH_COUNT)
        {
            char buf[160];
            snprintf(buf,sizeof(buf),"buildDefaultTechGraph() should build %d nodes, built %d.",
                     README_TECH_COUNT, readme.size());
            fail(buf); return 0;
        }
        if (readme.getRoot() != TECH_LANGUAGE)
        { fail("Language must be the root of the README graph."); return 0; }

        // Every codes.h technology is present exactly once, carries its own code as its
        // depCode, and the wiring matches the README table's dependency count.
        int edges = 0;
        for (int code = TECH_FIRST; code <= TECH_LAST; code++)
        {
            const Tech* t = readme.getTech(code);
            if (t == nullptr || t->depCode != code)
            {
                char buf[160];
                snprintf(buf,sizeof(buf),"technology 0x%02x is missing from the graph or does not carry its own code as depCode.", code);
                fail(buf); return 0;
            }
            edges += (int)t->inputs.size();
        }
        if (edges != README_DEPENDENCY_COUNT)
        {
            char buf[160];
            snprintf(buf,sizeof(buf),"README.md declares %d dependencies but the graph wired %d edges.",
                     README_DEPENDENCY_COUNT, edges);
            fail(buf); return 0;
        }
        if (readme.getTech(TECH_MILITARY_TRADITION)->inputs.size() != 7)
        { fail("Military Tradition should have 7 parents per the README table."); return 0; }
        if (readme.getTech(TECH_LANGUAGE)->inputs.size() != 0)
        { fail("Language is the root -- it should have no parents."); return 0; }

        // README.md's per-dependency weights, on the TECH_DEFAULT_WEIGHT scale: a "(1.0)"
        // dependency stores exactly the default, a "(0.8)" stores 0.8 of it.
        if (fabs(readme.getWeight(TECH_LANGUAGE, TECH_HUNTING) - TECH_DEFAULT_WEIGHT) > 0.0001f)
        { fail("Language -> Hunting is (1.0) in README.md: it should store TECH_DEFAULT_WEIGHT."); return 0; }
        if (fabs(readme.getWeight(TECH_LANGUAGE, TECH_ARCHERY) - 0.8f*TECH_DEFAULT_WEIGHT) > 0.0001f)
        { fail("Language -> Archery is (0.8) in README.md: it should store 0.8 * TECH_DEFAULT_WEIGHT."); return 0; }
        if (fabs(readme.getWeight(TECH_MATHEMATICS, TECH_MUSIC) - 0.2f*TECH_DEFAULT_WEIGHT) > 0.0001f)
        { fail("Mathematics -> Music is (0.2) in README.md, the smallest factor in the table."); return 0; }
        // Every weight must be a factor OF the default -- nothing above it, nothing at zero.
        for (int code = TECH_FIRST; code <= TECH_LAST; code++)
            for (const TechEdge& e : readme.getTech(code)->inputs)
                if (e.weight <= 0.0f || e.weight > TECH_DEFAULT_WEIGHT + 0.0001f)
                {
                    char buf[160];
                    snprintf(buf,sizeof(buf),"edge 0x%02x -> 0x%02x has weight %.4f, outside (0, TECH_DEFAULT_WEIGHT].",
                             e.from, code, e.weight);
                    fail(buf); return 0;
                }

        // Depth and the exponential bias that rides on it.
        if (readme.getDepth(TECH_LANGUAGE) != 0)
        { fail("The root technology sits at depth 0."); return 0; }
        if (readme.getDepth(TECH_HUNTING) != 1 || readme.getDepth(TECH_ARCHERY) != 2)
        { fail("Hunting is one hop from Language, Archery two (through Hunting)."); return 0; }
        if (readme.getDepth(TECH_INDUSTRIALIZATION) != 12)
        { fail("Industrialization is the deepest technology in README.md, 12 hops out."); return 0; }
        // Military Tradition has Warrior Code (2 hops) among its parents, but the LONGEST path
        // is what prices it -- otherwise a late technology with one shallow parent stays cheap.
        if (readme.getDepth(TECH_MILITARY_TRADITION) != 11)
        { fail("Military Tradition should be priced by its LONGEST path (11), not its shortest (2)."); return 0; }
        for (int code = TECH_FIRST; code <= TECH_LAST; code++)
        {
            const Tech* t = readme.getTech(code);
            float expected = powf(TECH_BIAS_BASE, (float)t->depth);
            if (fabs(t->bias - expected) > 0.001f * expected + 0.0001f)
            {
                char buf[160];
                snprintf(buf,sizeof(buf),"'%s' (depth %d) has bias %.3f, expected TECH_BIAS_BASE^depth = %.3f.",
                         t->name.c_str(), t->depth, t->bias, expected);
                fail(buf); return 0;
            }
        }

        // Two factions, each with its own copy of the graph (same weights -- they are data now;
        // what diverges is what each faction invests in).
        DependencyEvaluationEngine techdee;
        TechTree tree;
        tree.reset(2, readme);

        if (tree.factionCount() != 2)
        { fail("TechTree::reset(2, ...) should build two graphs."); return 0; }
        if (!tree.graph(0).isDiscovered(TECH_LANGUAGE) || !tree.graph(1).isDiscovered(TECH_LANGUAGE))
        { fail("Every faction must start knowing the root technology."); return 0; }
        if (tree.graph(0).isDiscovered(TECH_ALPHABET))
        { fail("A faction should start with the root only."); return 0; }

        // Turn 1: dump enough SCIENCE into Language that every one of its 13 children fires
        // (the smallest factor out of Language is 0.8, and even a depth-2 bias is only 4).
        std::unordered_map<int,int> plan;
        plan[TECH_LANGUAGE] = 1000;
        std::vector<int> got = tree.advance(0, plan, techdee);

        if ((int)got.size() != LANGUAGE_CHILD_COUNT)
        {
            char buf[160];
            snprintf(buf,sizeof(buf),"Language has %d direct children in the README table; %d fired.",
                     LANGUAGE_CHILD_COUNT, (int)got.size());
            fail(buf); return 0;
        }
        if (!contains(got,TECH_HUNTING) || !contains(got,TECH_ALPHABET) || !contains(got,TECH_POTTERY) ||
            !contains(got,TECH_ARCHERY) || !contains(got,TECH_CEREMONIAL_BURIAL))
        { fail("Turn 1 should have discovered Language's direct children."); return 0; }
        if (tree.graph(0).isDiscovered(TECH_WRITING))
        { fail("Writing is two layers deep -- it must not fire on turn 1."); return 0; }

        // Discovery activated the codes.h dependency codes, at FACTION scope -- which is what
        // the gated BuildableFactories (Granary/Barracks/Chariot/...) actually read.
        if (!techdee.verifyDep(factionContext(0), TECH_POTTERY) ||
            !techdee.verifyDep(factionContext(0), TECH_ALPHABET) ||
            !techdee.verifyDep(factionContext(0), TECH_THE_WHEEL) ||
            !techdee.verifyDep(factionContext(0), TECH_MINING))
        { fail("A discovered technology must register its codes.h code in the DEE at faction scope."); return 0; }
        if (techdee.verifyDep(factionContext(0), TECH_INDUSTRIALIZATION))
        { fail("Industrialization is undiscovered -- its dep code must NOT be registered."); return 0; }

        // Frontier bookkeeping at scale: Language is spent (every child known), Mining is a
        // leaf nothing depends on, Alphabet still leads somewhere.
        if (tree.graph(0).getFrontier().count(TECH_LANGUAGE) != 0)
        { fail("Language should leave the Frontier once all of its children are discovered."); return 0; }
        if (tree.graph(0).getFrontier().count(TECH_MINING) != 0)
        { fail("Mining is a leaf in the README table -- it should not sit in the Frontier."); return 0; }
        if (tree.graph(0).getFrontier().count(TECH_ALPHABET) != 1)
        { fail("Alphabet still has undiscovered children -- it belongs in the Frontier."); return 0; }
        if (tree.graph(0).getNext().count(TECH_WRITING) != 1)
        { fail("Writing fans out of Alphabet -- it should be a Next candidate."); return 0; }

        // Turn 2: pour into Alphabet -> everything reachable from it in one hop fires.
        plan.clear();
        plan[TECH_ALPHABET] = 1000;
        got = tree.advance(0, plan, techdee);

        if (!contains(got,TECH_WRITING) || !contains(got,TECH_MATHEMATICS) || !contains(got,TECH_MAP_MAKING))
        { fail("Turn 2 (all SCIENCE into Alphabet) should have fired Writing, Mathematics and Map Making."); return 0; }
        if (!techdee.verifyDep(factionContext(0), TECH_WRITING) ||
            !techdee.verifyDep(factionContext(0), TECH_MAP_MAKING))
        { fail("Turn 2's discoveries did not reach the DEE."); return 0; }

        // Investing into something the faction does not know is a no-op, not a discovery.
        plan.clear();
        plan[TECH_INDUSTRIALIZATION] = 100000;
        got = tree.advance(0, plan, techdee);
        if (contains(got, TECH_INDUSTRIALIZATION) || tree.graph(0).isDiscovered(TECH_INDUSTRIALIZATION))
        { fail("SCIENCE poured into an undiscovered tech must be ignored, not shortcut a discovery."); return 0; }

        // One graph per faction: faction 1 was never researched, so it still knows only the root.
        if (tree.graph(1).isDiscovered(TECH_ALPHABET) || tree.graph(1).isDiscovered(TECH_HUNTING))
        { fail("Faction 1 must not inherit faction 0's discoveries -- one graph per faction."); return 0; }
        if (techdee.verifyDep(factionContext(1), TECH_ALPHABET))
        { fail("Faction 0's discoveries must not register against faction 1's context."); return 0; }
    }

    isdone = true;
    haspassed = true;
    return 0;
}

std::string TestCase_058::title()
{
    return std::string("Tech tree scaffold (technologies.{h,cpp}): MLP-style tech GRAPH -- invest SCIENCE into Frontier techs, sigmoid(SUM(science*w)-bias) fires Next techs, Frontier/Next bookkeeping, and discovery registers the codes.h dep code at faction scope. Driven with the graph the game ships (buildDefaultTechGraph), whose codes live in codes.h.");
}

bool TestCase_058::done()   { return isdone; }
bool TestCase_058::passed() { return haspassed; }
std::string TestCase_058::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_058();
}
