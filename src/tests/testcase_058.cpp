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

// ---- README.md:307 as data ------------------------------------------------------------
// The technology codes themselves are codes.h's TECH_* (they ARE the README codes now), so a
// node's graph id and the dependency code it registers on discovery are the same number.

struct ReadmeTech { int code; const char* name; };

static const ReadmeTech README_TECHS[] = {
    { TECH_LANGUAGE,           "Language"           },
    { TECH_HUNTING,            "Hunting"            },
    { TECH_AGRICULTURE,        "Agriculture"        },
    { TECH_FISHING,            "Fishing"            },
    { TECH_MINING,             "Mining"             },
    { TECH_MASONRY,            "Masonry"            },
    { TECH_THE_WHEEL,          "The Wheel"          },
    { TECH_ARCHERY,            "Archery"            },
    { TECH_WARRIOR_CODE,       "Warrior Code"       },
    { TECH_BRONZE_WORKING,     "Bronze Working"     },
    { TECH_ANIMAL_HUSBANDRY,   "Animal Husbandry"   },
    { TECH_POTTERY,            "Pottery"            },
    { TECH_ALPHABET,           "Alphabet"           },
    { TECH_CEREMONIAL_BURIAL,  "Ceremonial Burial"  },
    { TECH_WRITING,            "Writing"            },
    { TECH_MATHEMATICS,        "Mathematics"        },
    { TECH_IRON_WORKING,       "Iron Working"       },
    { TECH_HORSEBACK_RIDING,   "Horseback Riding"   },
    { TECH_CONSTRUCTION,       "Construction"       },
    { TECH_CURRENCY,           "Currency"           },
    { TECH_MYSTICISM,          "Mysticism"          },
    { TECH_MAP_MAKING,         "Map Making"         },
    { TECH_POLYTHEISM,         "Polytheism"         },
    { TECH_LITERATURE,         "Literature"         },
    { TECH_CODE_OF_LAWS,       "Code of Laws"       },
    { TECH_PHILOSOPHY,         "Philosophy"         },
    { TECH_METAL_CASTING,      "Metal Casting"      },
    { TECH_MONOTHEISM,         "Monotheism"         },
    { TECH_REPUBLIC,           "Republic"           },
    { TECH_MONARCHY,           "Monarchy"           },
    { TECH_FEUDALISM,          "Feudalism"          },
    { TECH_SHIP_BUILDING,      "Ship Building"      },
    { TECH_THEOLOGY,           "Theology"           },
    { TECH_EDUCATION,          "Education"          },
    { TECH_ASTRONOMY,          "Astronomy"          },
    { TECH_BANKING,            "Banking"            },
    { TECH_CHIVALRY,           "Chivalry"           },
    { TECH_PHYSICS,            "Physics"            },
    { TECH_GUNPOWDER,          "Gunpowder"          },
    { TECH_MAGNETISM,          "Magnetism"          },
    { TECH_CHEMISTRY,          "Chemistry"          },
    { TECH_METALLURGY,         "Metallurgy"         },
    { TECH_CHARTERS,           "Charters"           },
    { TECH_MUSIC,              "Music"              },
    { TECH_INDUSTRIALIZATION,  "Industrialization"  },
    { TECH_MILITARY_TRADITION, "Military Tradition" }
};

// One row per (dependency -> technology) pair of the README table.
struct ReadmeDep { int from; int to; };

static const ReadmeDep README_DEPS[] = {
    { TECH_LANGUAGE, TECH_HUNTING },
    { TECH_LANGUAGE, TECH_AGRICULTURE },
    { TECH_LANGUAGE, TECH_FISHING },
    { TECH_LANGUAGE, TECH_MINING },
    { TECH_LANGUAGE, TECH_MASONRY },
    { TECH_LANGUAGE, TECH_THE_WHEEL },
    { TECH_LANGUAGE, TECH_ARCHERY },           { TECH_HUNTING, TECH_ARCHERY },
    { TECH_LANGUAGE, TECH_WARRIOR_CODE },      { TECH_HUNTING, TECH_WARRIOR_CODE },
    { TECH_LANGUAGE, TECH_BRONZE_WORKING },    { TECH_HUNTING, TECH_BRONZE_WORKING },
    { TECH_LANGUAGE, TECH_ANIMAL_HUSBANDRY },  { TECH_HUNTING, TECH_ANIMAL_HUSBANDRY },
    { TECH_LANGUAGE, TECH_POTTERY },           { TECH_AGRICULTURE, TECH_POTTERY },
    { TECH_LANGUAGE, TECH_ALPHABET },
    { TECH_LANGUAGE, TECH_CEREMONIAL_BURIAL },

    { TECH_ALPHABET, TECH_WRITING },
    { TECH_MASONRY, TECH_MATHEMATICS },        { TECH_ALPHABET, TECH_MATHEMATICS },
    { TECH_BRONZE_WORKING, TECH_IRON_WORKING },
    { TECH_WARRIOR_CODE, TECH_HORSEBACK_RIDING }, { TECH_ANIMAL_HUSBANDRY, TECH_HORSEBACK_RIDING },
    { TECH_ARCHERY, TECH_HORSEBACK_RIDING },
    { TECH_MASONRY, TECH_CONSTRUCTION },       { TECH_IRON_WORKING, TECH_CONSTRUCTION },
    { TECH_MATHEMATICS, TECH_CONSTRUCTION },   { TECH_THE_WHEEL, TECH_CONSTRUCTION },
    { TECH_IRON_WORKING, TECH_CURRENCY },      { TECH_MATHEMATICS, TECH_CURRENCY },
    { TECH_CEREMONIAL_BURIAL, TECH_MYSTICISM },
    { TECH_FISHING, TECH_MAP_MAKING },         { TECH_ALPHABET, TECH_MAP_MAKING },
    { TECH_POTTERY, TECH_MAP_MAKING },
    { TECH_WARRIOR_CODE, TECH_POLYTHEISM },    { TECH_MYSTICISM, TECH_POLYTHEISM },
    { TECH_WRITING, TECH_LITERATURE },         { TECH_ALPHABET, TECH_LITERATURE },
    { TECH_WRITING, TECH_CODE_OF_LAWS },       { TECH_WARRIOR_CODE, TECH_CODE_OF_LAWS },
    { TECH_MATHEMATICS, TECH_PHILOSOPHY },     { TECH_WRITING, TECH_PHILOSOPHY },
    { TECH_IRON_WORKING, TECH_METAL_CASTING }, { TECH_CONSTRUCTION, TECH_METAL_CASTING },
    { TECH_MYSTICISM, TECH_MONOTHEISM },       { TECH_POLYTHEISM, TECH_MONOTHEISM },
    { TECH_CODE_OF_LAWS, TECH_REPUBLIC },      { TECH_PHILOSOPHY, TECH_REPUBLIC },
    { TECH_POLYTHEISM, TECH_MONARCHY },        { TECH_MONOTHEISM, TECH_MONARCHY },
    { TECH_ARCHERY, TECH_FEUDALISM },          { TECH_MONARCHY, TECH_FEUDALISM },
    { TECH_CURRENCY, TECH_FEUDALISM },
    { TECH_CONSTRUCTION, TECH_SHIP_BUILDING }, { TECH_MAP_MAKING, TECH_SHIP_BUILDING },
    { TECH_METAL_CASTING, TECH_SHIP_BUILDING },{ TECH_FEUDALISM, TECH_SHIP_BUILDING },
    { TECH_MONOTHEISM, TECH_THEOLOGY },        { TECH_PHILOSOPHY, TECH_THEOLOGY },
    { TECH_ALPHABET, TECH_EDUCATION },         { TECH_LITERATURE, TECH_EDUCATION },
    { TECH_REPUBLIC, TECH_EDUCATION },         { TECH_THEOLOGY, TECH_EDUCATION },
    { TECH_ALPHABET, TECH_ASTRONOMY },         { TECH_MATHEMATICS, TECH_ASTRONOMY },
    { TECH_MAP_MAKING, TECH_ASTRONOMY },       { TECH_CEREMONIAL_BURIAL, TECH_ASTRONOMY },
    { TECH_SHIP_BUILDING, TECH_ASTRONOMY },    { TECH_EDUCATION, TECH_ASTRONOMY },
    { TECH_CURRENCY, TECH_BANKING },           { TECH_CODE_OF_LAWS, TECH_BANKING },
    { TECH_EDUCATION, TECH_BANKING },
    { TECH_MONOTHEISM, TECH_CHIVALRY },        { TECH_FEUDALISM, TECH_CHIVALRY },
    { TECH_MONARCHY, TECH_CHIVALRY },          { TECH_THEOLOGY, TECH_CHIVALRY },
    { TECH_ALPHABET, TECH_PHYSICS },           { TECH_MATHEMATICS, TECH_PHYSICS },
    { TECH_IRON_WORKING, TECH_PHYSICS },       { TECH_ASTRONOMY, TECH_PHYSICS },
    { TECH_POTTERY, TECH_GUNPOWDER },          { TECH_CEREMONIAL_BURIAL, TECH_GUNPOWDER },
    { TECH_FEUDALISM, TECH_GUNPOWDER },
    { TECH_MAP_MAKING, TECH_MAGNETISM },       { TECH_IRON_WORKING, TECH_MAGNETISM },
    { TECH_SHIP_BUILDING, TECH_MAGNETISM },    { TECH_ASTRONOMY, TECH_MAGNETISM },
    { TECH_CEREMONIAL_BURIAL, TECH_CHEMISTRY },{ TECH_POTTERY, TECH_CHEMISTRY },
    { TECH_GUNPOWDER, TECH_CHEMISTRY },        { TECH_PHYSICS, TECH_CHEMISTRY },
    { TECH_IRON_WORKING, TECH_METALLURGY },    { TECH_BRONZE_WORKING, TECH_METALLURGY },
    { TECH_METAL_CASTING, TECH_METALLURGY },   { TECH_CHEMISTRY, TECH_METALLURGY },
    { TECH_CURRENCY, TECH_CHARTERS },          { TECH_BANKING, TECH_CHARTERS },
    { TECH_WRITING, TECH_CHARTERS },           { TECH_CODE_OF_LAWS, TECH_CHARTERS },
    { TECH_PHILOSOPHY, TECH_CHARTERS },
    { TECH_CEREMONIAL_BURIAL, TECH_MUSIC },    { TECH_MATHEMATICS, TECH_MUSIC },
    { TECH_LITERATURE, TECH_MUSIC },           { TECH_MYSTICISM, TECH_MUSIC },
    { TECH_EDUCATION, TECH_MUSIC },
    { TECH_CHARTERS, TECH_INDUSTRIALIZATION }, { TECH_MAGNETISM, TECH_INDUSTRIALIZATION },
    { TECH_CODE_OF_LAWS, TECH_INDUSTRIALIZATION }, { TECH_CURRENCY, TECH_INDUSTRIALIZATION },
    { TECH_METALLURGY, TECH_INDUSTRIALIZATION },
    { TECH_WARRIOR_CODE, TECH_MILITARY_TRADITION }, { TECH_HORSEBACK_RIDING, TECH_MILITARY_TRADITION },
    { TECH_LITERATURE, TECH_MILITARY_TRADITION },   { TECH_CHIVALRY, TECH_MILITARY_TRADITION },
    { TECH_EDUCATION, TECH_MILITARY_TRADITION },    { TECH_GUNPOWDER, TECH_MILITARY_TRADITION },
    { TECH_CHEMISTRY, TECH_MILITARY_TRADITION }
};

#define ARRAY_COUNT(a) ((int)(sizeof(a)/sizeof((a)[0])))

// The node id IS the codes.h dependency code for the game's graph, so each row supplies its
// code twice: once as the graph id, once as what discovery registers in the DEE.
static TechGraph buildReadmeGraph()
{
    TechGraph g;

    for (int i=0;i<ARRAY_COUNT(README_TECHS);i++)
        g.addTech(README_TECHS[i].code, README_TECHS[i].name, README_TECHS[i].code);

    g.setRoot(TECH_ROOT);

    for (int i=0;i<ARRAY_COUNT(README_DEPS);i++)
        g.addEdge(README_DEPS[i].from, README_DEPS[i].to);

    return g;
}

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

        // Random init must land inside the documented ranges.
        float wAB = g.getWeight(A,B);
        if (wAB < TECH_WEIGHT_MIN || wAB > TECH_WEIGHT_MAX)
        { fail("A randomly initialized edge weight is outside [TECH_WEIGHT_MIN, TECH_WEIGHT_MAX]."); return 0; }
        float bB = g.getBias(B);
        if (bB < TECH_BIAS_MIN || bB > TECH_BIAS_MAX)
        { fail("A randomly initialized bias is outside [TECH_BIAS_MIN, TECH_BIAS_MAX]."); return 0; }

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
        // codes.h now carries the whole README table, so its own bookkeeping must line up.
        if (ARRAY_COUNT(README_TECHS) != TECH_COUNT)
        {
            char buf[160];
            snprintf(buf,sizeof(buf),"codes.h says TECH_COUNT=%d but the README table lists %d technologies.",
                     (int)TECH_COUNT, ARRAY_COUNT(README_TECHS));
            fail(buf); return 0;
        }
        // Codes must be the contiguous run TECH_FIRST..TECH_LAST, in table order.
        for (int i=0;i<ARRAY_COUNT(README_TECHS);i++)
            if (README_TECHS[i].code != TECH_FIRST + i)
            {
                char buf[160];
                snprintf(buf,sizeof(buf),"codes.h technology codes are not contiguous: row %d ('%s') is 0x%02x.",
                         i, README_TECHS[i].name, README_TECHS[i].code);
                fail(buf); return 0;
            }
        if (TECH_ROOT != TECH_LANGUAGE)
        { fail("codes.h TECH_ROOT should be TECH_LANGUAGE."); return 0; }

        TechGraph readme = buildReadmeGraph();

        if (readme.size() != TECH_COUNT)
        {
            char buf[160];
            snprintf(buf,sizeof(buf),"The README tech table should build %d nodes, built %d.",
                     (int)TECH_COUNT, readme.size());
            fail(buf); return 0;
        }
        if (readme.getRoot() != TECH_LANGUAGE)
        { fail("Language must be the root of the README graph."); return 0; }

        // Every node registers its own code, and every declared dependency became an edge.
        for (int i=0;i<ARRAY_COUNT(README_TECHS);i++)
        {
            const Tech* t = readme.getTech(README_TECHS[i].code);
            if (t == nullptr || t->depCode != README_TECHS[i].code)
            {
                char buf[160];
                snprintf(buf,sizeof(buf),"'%s' should carry its own codes.h code as depCode.",
                         README_TECHS[i].name);
                fail(buf); return 0;
            }
        }
        int edges = 0;
        for (int i=0;i<ARRAY_COUNT(README_TECHS);i++)
            edges += (int)readme.getTech(README_TECHS[i].code)->inputs.size();
        if (edges != ARRAY_COUNT(README_DEPS))
        {
            char buf[160];
            snprintf(buf,sizeof(buf),"The README table declares %d dependencies but the graph wired %d edges.",
                     ARRAY_COUNT(README_DEPS), edges);
            fail(buf); return 0;
        }
        if (readme.getTech(TECH_MILITARY_TRADITION)->inputs.size() != 7)
        { fail("Military Tradition should have 7 parents per the README table."); return 0; }
        if (readme.getTech(TECH_LANGUAGE)->inputs.size() != 0)
        { fail("Language is the root -- it should have no parents."); return 0; }

        // Two factions, each with its own graph (and its own random weights).
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
        // regardless of how their weights were randomized (w >= TECH_WEIGHT_MIN = 0.1 > 0).
        std::unordered_map<int,int> plan;
        plan[TECH_LANGUAGE] = 1000;
        std::vector<int> got = tree.advance(0, plan, techdee);

        if (got.size() != 13)
        {
            char buf[160];
            snprintf(buf,sizeof(buf),"Language has 13 direct children in the README table; %d fired.",
                     (int)got.size());
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
        { fail("Language should leave the Frontier once all 13 of its children are discovered."); return 0; }
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
    return std::string("Tech tree scaffold (technologies.{h,cpp}): MLP-style tech GRAPH -- invest SCIENCE into Frontier techs, sigmoid(SUM(science*w)-bias) fires Next techs, Frontier/Next bookkeeping, and discovery registers the codes.h dep code at faction scope. Driven with the README.md:307 table, whose codes now live in codes.h.");
}

bool TestCase_058::done()   { return isdone; }
bool TestCase_058::passed() { return haspassed; }
std::string TestCase_058::failedMessage() { return message; }

TestCase *pickTestCase(int testcase)
{
    return new TestCase_058();
}
