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
// NOTE: this header/implementation is the SCAFFOLD only -- it deliberately carries no
// technology data. The concrete graph (README.md's tech table) is supplied by the caller via
// addTech/addEdge, so the game and the testcases build their own.
//
// There is one TechGraph per faction (TechTree below owns them): weights and biases are
// randomized per graph, so two civilizations researching the same investment plan discover
// things in a different order.

// Random-initialization range for edge weights and node biases, and the sigmoid output at
// which a technology is considered discovered. @FIXME These need real balancing: with the
// current ranges a node fires after roughly half a dozen SCIENCE points invested in one
// parent, which is far too fast for a full game.
const float TECH_WEIGHT_MIN       = 0.1f;
const float TECH_WEIGHT_MAX       = 0.9f;
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

    // One research turn for one faction: `investments` is (techId -> SCIENCE) for this turn
    // (all of it must land on Frontier nodes; anything else is ignored), then the graph is
    // stepped and every newly discovered technology's depCode is registered in `dee`.
    // Returns the ids discovered this turn.
    std::vector<int> advance(int factionId,
                             const std::unordered_map<int,int>& investments,
                             DependencyEvaluationEngine& dee);

private:
    std::vector<TechGraph> graphs;
};

#endif // TECHNOLOGIES_H
