#ifndef TECHNOLOGIES_H
#define TECHNOLOGIES_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "dee.h"

// Tech "tree" scaffold (README.md, "Science").
//
// It is NOT a tree: it is a directed acyclic GRAPH wired like a multilayered perceptron.
// Every node is one technology; every edge (parent -> child) carries a weight. A node also
// carries a bias. Instead of picking the advance you want, a faction pours its accumulated
// SCIENCE into technologies it ALREADY knows, and that investment propagates forward:
//
//      activation(t) = sigmoid( SUM over parents p of ( science(p) * w(p,t) ) - bias(t) )
//
// (equation 1 in the task). When that activation crosses TECH_FIRING_THRESHOLD the
// technology fires -- it is discovered, and its codes.h dependency code is registered in the
// Dependency Evaluation Engine at FACTION scope, which is what unlocks buildables.
//
// Three sets drive a turn:
//   discovered -- everything the faction knows. ROOT ("Language") starts discovered.
//   Frontier   -- the discovered techs that still have at least one undiscovered child.
//                 SCIENCE can ONLY be invested into these (you cannot research what you do
//                 not know yet). A discovered tech whose children are all discovered (or
//                 that has no children at all) is dropped from the Frontier.
//   Next       -- the undiscovered techs fanning out of the Frontier: this turn's candidates.
//                 A Next tech that fires is moved into the Frontier.
//
// The TechGraph/TechTree machinery itself carries no technology data -- a graph is built by
// the caller through addTech/addEdge, so a test can use any shape it likes. The GAME's graph
// (README.md's tech table) is buildDefaultTechGraph() at the bottom of this header.
//
// There is one TechGraph per faction (TechTree below owns them): weights and biases are
// randomized per graph, so two civilizations researching the same investment plan discover
// things in a different order.

// Random-initialization range for edge weights and node biases, and the sigmoid output at
// which a technology is considered discovered.
//
// Firing needs sigmoid(science*w - bias) >= TECH_FIRING_THRESHOLD, i.e.
// science >= (logit(threshold) + bias) / w = (2.197 + bias) / w. Averaged over the ranges
// below that is ~74 SCIENCE per discovery (the weights were scaled down 10x from an earlier
// 0.1..0.9, which fired after only ~7). Widen/narrow the WEIGHT range to retune: expected
// cost scales as 1/w, while the bias only shifts it by a couple of points.
const float TECH_WEIGHT_MIN       = 0.01f;
const float TECH_WEIGHT_MAX       = 0.09f;
const float TECH_BIAS_MIN         = 0.1f;
const float TECH_BIAS_MAX         = 0.9f;
const float TECH_FIRING_THRESHOLD = 0.9f;

// A node that unlocks nothing in the Dependency Evaluation Engine. Real nodes carry one of
// codes.h's TECH_* codes, which ARE the README.md technology codes -- so for the game's graph
// Tech::id and Tech::depCode hold the same number. They stay two separate fields because the
// scaffold does not require it: a graph may use any node ids it likes (see testcase_058's
// small pinned-weight graph), and a node may legitimately gate nothing (TECH_NO_DEP_CODE).
const int TECH_NO_DEP_CODE = 0;

float techSigmoid(float x);

// One incoming edge: the parent feeding this node, and the weight on that connection.
struct TechEdge
{
    int   from = 0;
    float weight = 0.0f;
};

class Tech
{
public:
    int         id = 0;                        // node id in this graph (codes.h TECH_* for the game's graph)
    std::string name;
    int         depCode = TECH_NO_DEP_CODE;    // codes.h TECH_* registered in the DEE on discovery

    int   science    = 0;                      // SCIENCE invested INTO this node so far
    bool  discovered = false;
    float bias       = 0.0f;

    std::vector<TechEdge> inputs;              // parents (with their weights)
    std::vector<int>      outputs;             // fan-out: children this node feeds
};

class TechGraph
{
public:
    TechGraph();

    // ---- graph construction -------------------------------------------------------------
    // Adds a node. `id` is the caller's technology code; re-adding an existing id is ignored.
    void addTech(int id, const char* name, int depCode = TECH_NO_DEP_CODE);
    // Wires parent -> child with a freshly randomized weight. Both nodes must already exist;
    // a duplicate edge is ignored.
    void addEdge(int fromId, int toId);
    // The one technology every faction starts with ("Language").
    void setRoot(int id);
    int  getRoot() const;

    // Rerolls every weight and bias inside [TECH_WEIGHT_MIN..MAX] / [TECH_BIAS_MIN..MAX].
    // Called per faction so each one gets its own graph.
    void randomize();

    // Resets all progress: only the root is discovered, every node's science is 0, and the
    // Frontier/Next sets are seeded from the root. Weights and biases are left alone.
    void start();

    // ---- state --------------------------------------------------------------------------
    bool isDiscovered(int id) const;
    const std::unordered_set<int>& getFrontier() const;
    const std::unordered_set<int>& getNext() const;
    // The Frontier in graph insertion order. The set itself is unordered, so anything that
    // maps a position back to a technology (the research selector dialog) must use this.
    std::vector<int> getFrontierOrdered() const;

    Tech*       getTech(int id);
    const Tech* getTech(int id) const;
    std::vector<int> getTechIds() const;        // in insertion order
    int size() const;

    // ---- one turn -----------------------------------------------------------------------
    // Pours `amount` SCIENCE into a Frontier technology. Rejected (returns false) for a node
    // that is not in the Frontier -- you can only invest in what you already know. Investment
    // can be split across several Frontier nodes by calling this more than once.
    bool invest(int id, int amount);

    // Recomputes every Next candidate, promotes the ones that fire into the Frontier, then
    // prunes Frontier nodes that have nothing left to discover and rebuilds Next.
    // Returns the ids discovered by this call (empty if none fired).
    std::vector<int> step();

    // sigmoid( SUM(parent.science * w) - bias ). 0 for an unknown id.
    float activation(int id) const;

    // ---- tuning / testing hooks ---------------------------------------------------------
    float getWeight(int fromId, int toId) const;
    bool  setWeight(int fromId, int toId, float w);
    float getBias(int id) const;
    bool  setBias(int id, float b);

private:
    std::unordered_map<int, Tech> techs;
    std::vector<int> order;                    // insertion order, so iteration is stable
    int root = 0;

    std::unordered_set<int> frontier;
    std::unordered_set<int> next;

    bool hasUndiscoveredChild(int id) const;
    void rebuildNext();
    void pruneFrontier();
};

// One graph per faction, plus the bridge into the Dependency Evaluation Engine: whenever a
// technology is discovered its depCode is registered at factionContext(factionId), which is
// what the BuildableFactory dependency gates read.
class TechTree
{
public:
    // Gives every faction its own copy of `prototype` with freshly randomized weights/biases,
    // reset to "only the root is known".
    void reset(int factionCount, const TechGraph& prototype);

    int  factionCount() const;
    TechGraph&       graph(int factionId);
    const TechGraph& graph(int factionId) const;

    // ---- research target: the ONE technology a faction is currently pouring SCIENCE into --
    // 0 when nothing is selected.
    int  getResearchTarget(int factionId) const;
    // Accepted only for a technology currently in that faction's Frontier (you can only
    // research what you already know). Returns false otherwise, leaving the target alone.
    bool setResearchTarget(int factionId, int techId);
    // True when the faction has no usable target -- it never picked one, or the one it had
    // dropped out of the Frontier because everything that technology led to is now known.
    // That is the moment to ask the player again (or let the AI reroll).
    bool needsResearchTarget(int factionId) const;
    // A random Frontier technology: what an autoPlayer faction picks, and the fallback for a
    // human faction that has not answered the selector. 0 if the Frontier is empty.
    int  pickRandomTarget(int factionId);

    // SCIENCE handed to advance() while the faction had no valid target -- e.g. the player has
    // not answered the selector yet. It is held here and spent whole on the next investment,
    // so a year's research is never silently thrown away while the dialog is open.
    int  getPendingScience(int factionId) const;

    // One research turn for one faction: `investments` is (techId -> SCIENCE) for this turn
    // (all of it must land on Frontier nodes; anything else is ignored), then the graph is
    // stepped and every newly discovered technology's depCode is registered in `dee`.
    // Returns the ids discovered this turn.
    std::vector<int> advance(int factionId,
                             const std::unordered_map<int,int>& investments,
                             DependencyEvaluationEngine& dee);
    // The same, pouring a year's SCIENCE into whatever the faction currently has selected.
    std::vector<int> advance(int factionId, int science, DependencyEvaluationEngine& dee);

private:
    std::vector<TechGraph> graphs;
    std::vector<int>       targets;      // per faction, 0 = nothing selected
    std::vector<int>       pending;      // per faction, SCIENCE banked awaiting a target
};

// The README.md "Science" table as a graph: every technology in codes.h (TECH_FIRST..TECH_LAST)
// wired by the dependencies that table declares, rooted at TECH_ROOT (Language). This is the
// one place the game's technology data lives.
TechGraph buildDefaultTechGraph();

// Shared game/simulation setup: builds the default graph, gives every faction its own
// randomized copy with only the root discovered, and registers the root's dependency code for
// each of them (every faction starts knowing Language). gamekernel.cpp and simulate.cpp both
// call this -- it lives here because gamekernel.cpp is not linked into the simulator.
void initTechnologies(TechTree& tree, int factionCount, DependencyEvaluationEngine& dee);

#endif // TECHNOLOGIES_H
