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
