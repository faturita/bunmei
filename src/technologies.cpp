#include <cmath>

#include "math/yamathutil.h"
#include "technologies.h"

float techSigmoid(float x)
{
    return 1.0f / (1.0f + expf(-x));
}

// getRandomInteger is the project's single seeded generator (yamathutil.cpp; -seed reseeds
// it), so derive the float from it instead of introducing a second RNG that -seed would not
// reach.
static float randomInRange(float lo, float hi)
{
    return lo + (hi - lo) * ((float)getRandomInteger(0,1000) / 1000.0f);
}

TechGraph::TechGraph()
{
}

void TechGraph::addTech(int id, const char* name, int depCode)
{
    if (techs.find(id) != techs.end())
        return;

    Tech t;
    t.id      = id;
    t.name    = name;
    t.depCode = depCode;
    t.bias    = randomInRange(TECH_BIAS_MIN, TECH_BIAS_MAX);

    techs[id] = t;
    order.push_back(id);
}

void TechGraph::addEdge(int fromId, int toId)
{
    auto from = techs.find(fromId);
    auto to   = techs.find(toId);
    if (from == techs.end() || to == techs.end() || fromId == toId)
        return;

    for (const TechEdge& e : to->second.inputs)
        if (e.from == fromId)
            return;                       // already wired

    TechEdge e;
    e.from   = fromId;
    e.weight = randomInRange(TECH_WEIGHT_MIN, TECH_WEIGHT_MAX);
    to->second.inputs.push_back(e);

    from->second.outputs.push_back(toId);
}

void TechGraph::setRoot(int id)
{
    root = id;
}

int TechGraph::getRoot() const
{
    return root;
}

void TechGraph::randomize()
{
    for (int id : order)
    {
        Tech& t = techs[id];
        t.bias = randomInRange(TECH_BIAS_MIN, TECH_BIAS_MAX);
        for (TechEdge& e : t.inputs)
            e.weight = randomInRange(TECH_WEIGHT_MIN, TECH_WEIGHT_MAX);
    }
}

void TechGraph::start()
{
    for (int id : order)
    {
        Tech& t = techs[id];
        t.science    = 0;
        t.discovered = false;
    }

    frontier.clear();
    next.clear();

    auto it = techs.find(root);
    if (it == techs.end())
        return;                            // no root set: nothing is known, nothing to research

    it->second.discovered = true;
    frontier.insert(root);

    pruneFrontier();
    rebuildNext();
}

bool TechGraph::isDiscovered(int id) const
{
    auto it = techs.find(id);
    return it != techs.end() && it->second.discovered;
}

const std::unordered_set<int>& TechGraph::getFrontier() const
{
    return frontier;
}

const std::unordered_set<int>& TechGraph::getNext() const
{
    return next;
}

std::vector<int> TechGraph::getFrontierOrdered() const
{
    std::vector<int> out;
    for (int id : order)
        if (frontier.find(id) != frontier.end())
            out.push_back(id);
    return out;
}

Tech* TechGraph::getTech(int id)
{
    auto it = techs.find(id);
    return it == techs.end() ? nullptr : &it->second;
}

const Tech* TechGraph::getTech(int id) const
{
    auto it = techs.find(id);
    return it == techs.end() ? nullptr : &it->second;
}

std::vector<int> TechGraph::getTechIds() const
{
    return order;
}

int TechGraph::size() const
{
    return (int)order.size();
}

bool TechGraph::invest(int id, int amount)
{
    if (amount <= 0)
        return false;
    if (frontier.find(id) == frontier.end())
        return false;                      // only what the faction already knows can be researched

    techs[id].science += amount;
    return true;
}

// sigmoid( SUM(parent.science * w) - bias ). Undiscovered parents contribute nothing on their
// own -- their science is 0 -- so summing over EVERY parent is the same as summing over the
// discovered ones, and needs no extra bookkeeping.
float TechGraph::activation(int id) const
{
    auto it = techs.find(id);
    if (it == techs.end())
        return 0.0f;

    const Tech& t = it->second;

    float sum = 0.0f;
    for (const TechEdge& e : t.inputs)
    {
        auto p = techs.find(e.from);
        if (p != techs.end())
            sum += (float)p->second.science * e.weight;
    }

    return techSigmoid(sum - t.bias);
}

bool TechGraph::hasUndiscoveredChild(int id) const
{
    auto it = techs.find(id);
    if (it == techs.end())
        return false;

    for (int childId : it->second.outputs)
    {
        auto c = techs.find(childId);
        if (c != techs.end() && !c->second.discovered)
            return true;
    }
    return false;
}

// Next = every undiscovered technology fanning out of a Frontier node.
void TechGraph::rebuildNext()
{
    next.clear();
    for (int id : frontier)
    {
        const Tech& t = techs[id];
        for (int childId : t.outputs)
        {
            auto c = techs.find(childId);
            if (c != techs.end() && !c->second.discovered)
                next.insert(childId);
        }
    }
}

// A discovered technology with nothing undiscovered left to lead to cannot be researched any
// further, so it leaves the Frontier (it stays discovered, and its accumulated science keeps
// feeding its edges).
void TechGraph::pruneFrontier()
{
    std::vector<int> spent;
    for (int id : frontier)
        if (!hasUndiscoveredChild(id))
            spent.push_back(id);

    for (int id : spent)
        frontier.erase(id);
}

std::vector<int> TechGraph::step()
{
    std::vector<int> discovered;

    // Candidates are recomputed first so a caller that only ever calls step() still sees a
    // correct Next set even if the graph was edited since the last turn.
    rebuildNext();

    // Snapshot: firing nodes are promoted into the Frontier, which would otherwise mutate
    // `next` under the loop.
    std::vector<int> candidates(next.begin(), next.end());
    for (int id : candidates)
    {
        if (activation(id) < TECH_FIRING_THRESHOLD)
            continue;

        techs[id].discovered = true;
        next.erase(id);
        frontier.insert(id);
        discovered.push_back(id);
    }

    // Newly discovered nodes may have completed a parent's fan-out, and they open their own.
    pruneFrontier();
    rebuildNext();

    return discovered;
}

float TechGraph::getWeight(int fromId, int toId) const
{
    auto to = techs.find(toId);
    if (to == techs.end())
        return 0.0f;

    for (const TechEdge& e : to->second.inputs)
        if (e.from == fromId)
            return e.weight;

    return 0.0f;
}

bool TechGraph::setWeight(int fromId, int toId, float w)
{
    auto to = techs.find(toId);
    if (to == techs.end())
        return false;

    for (TechEdge& e : to->second.inputs)
        if (e.from == fromId)
        {
            e.weight = w;
            return true;
        }

    return false;
}

float TechGraph::getBias(int id) const
{
    auto it = techs.find(id);
    return it == techs.end() ? 0.0f : it->second.bias;
}

bool TechGraph::setBias(int id, float b)
{
    auto it = techs.find(id);
    if (it == techs.end())
        return false;

    it->second.bias = b;
    return true;
}

// ---------------------------------------------------------------------------------------

void TechTree::reset(int factionCount, const TechGraph& prototype)
{
    graphs.clear();
    targets.clear();
    pending.clear();
    if (factionCount < 0)
        factionCount = 0;

    graphs.reserve(factionCount);
    for (int i=0;i<factionCount;i++)
    {
        TechGraph g = prototype;      // same shape ...
        g.randomize();                // ... its own weights and biases
        g.start();
        graphs.push_back(g);
    }
    targets.assign(factionCount, 0);
    pending.assign(factionCount, 0);
}

int TechTree::getPendingScience(int factionId) const
{
    if (factionId < 0 || factionId >= (int)pending.size())
        return 0;
    return pending[factionId];
}

int TechTree::getResearchTarget(int factionId) const
{
    if (factionId < 0 || factionId >= (int)targets.size())
        return 0;
    return targets[factionId];
}

bool TechTree::setResearchTarget(int factionId, int techId)
{
    if (factionId < 0 || factionId >= (int)graphs.size())
        return false;
    if (graphs[factionId].getFrontier().count(techId) == 0)
        return false;

    targets[factionId] = techId;
    return true;
}

bool TechTree::needsResearchTarget(int factionId) const
{
    if (factionId < 0 || factionId >= (int)graphs.size())
        return false;
    // Nothing left to research at all is not "needs a target" -- there is nothing to ask for.
    if (graphs[factionId].getFrontier().empty())
        return false;

    return graphs[factionId].getFrontier().count(targets[factionId]) == 0;
}

int TechTree::pickRandomTarget(int factionId)
{
    if (factionId < 0 || factionId >= (int)graphs.size())
        return 0;

    std::vector<int> choices = graphs[factionId].getFrontierOrdered();
    if (choices.empty())
        return 0;

    int pick = choices[getRandomInteger(0, (int)choices.size() - 1)];
    targets[factionId] = pick;
    return pick;
}

int TechTree::factionCount() const
{
    return (int)graphs.size();
}

TechGraph& TechTree::graph(int factionId)
{
    return graphs[factionId];
}

const TechGraph& TechTree::graph(int factionId) const
{
    return graphs[factionId];
}

std::vector<int> TechTree::advance(int factionId,
                                   const std::unordered_map<int,int>& investments,
                                   DependencyEvaluationEngine& dee)
{
    if (factionId < 0 || factionId >= (int)graphs.size())
        return std::vector<int>();

    TechGraph& g = graphs[factionId];

    for (const auto& kv : investments)
        g.invest(kv.first, kv.second);    // non-Frontier targets are rejected inside invest()

    std::vector<int> discovered = g.step();

    // The whole point of discovering something: open the buildables gated behind it.
    for (int id : discovered)
    {
        Tech* t = g.getTech(id);
        if (t != nullptr && t->depCode != TECH_NO_DEP_CODE)
            dee.regDep(factionContext(factionId), t->depCode);
    }

    return discovered;
}

std::vector<int> TechTree::advance(int factionId, int science, DependencyEvaluationEngine& dee)
{
    if (factionId < 0 || factionId >= (int)graphs.size())
        return std::vector<int>();

    // Bank first, spend second. The caller has already emptied its cities' SCIENCE counters, so
    // if there is no usable target yet -- the player has been prompted and has not answered --
    // the year's research would otherwise just vanish. It waits here instead and goes in whole
    // with the next investment.
    if (science > 0)
        pending[factionId] += science;

    std::unordered_map<int,int> investments;

    int target = getResearchTarget(factionId);
    if (target != 0 && pending[factionId] > 0 && graphs[factionId].getFrontier().count(target) > 0)
    {
        investments[target] = pending[factionId];
        pending[factionId] = 0;
    }

    // Still steps with an empty plan: a node can only fire off science already banked, but
    // stepping keeps the Frontier/Next bookkeeping current after e.g. a captured city.
    return advance(factionId, investments, dee);
}

// ---------------------------------------------------------------------------------------
// README.md's "Science" table. One row per technology, then one row per declared dependency.
// The node id IS the codes.h dependency code (see codes.h), so each technology registers its
// own code when it fires.

struct DefaultTechRow { int code; const char* name; };

static const DefaultTechRow DEFAULT_TECHS[] = {
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

struct DefaultTechDep { int from; int to; };

static const DefaultTechDep DEFAULT_DEPS[] = {
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

TechGraph buildDefaultTechGraph()
{
    TechGraph g;

    for (unsigned int i=0;i<sizeof(DEFAULT_TECHS)/sizeof(DEFAULT_TECHS[0]);i++)
        g.addTech(DEFAULT_TECHS[i].code, DEFAULT_TECHS[i].name, DEFAULT_TECHS[i].code);

    g.setRoot(TECH_ROOT);

    for (unsigned int i=0;i<sizeof(DEFAULT_DEPS)/sizeof(DEFAULT_DEPS[0]);i++)
        g.addEdge(DEFAULT_DEPS[i].from, DEFAULT_DEPS[i].to);

    return g;
}

void initTechnologies(TechTree& tree, int factionCount, DependencyEvaluationEngine& dee)
{
    tree.reset(factionCount, buildDefaultTechGraph());

    // Every faction starts knowing the root (TechGraph::start() marks it discovered); the
    // DEE has to be told too, or a buildable gated on Language would never appear.
    for (int f=0;f<factionCount;f++)
    {
        const Tech* rootTech = tree.graph(f).getTech(TECH_ROOT);
        if (rootTech != nullptr && rootTech->depCode != TECH_NO_DEP_CODE)
            dee.regDep(factionContext(f), rootTech->depCode);
    }
}
