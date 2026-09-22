#ifndef BUILDABLE_H
#define BUILDABLE_H

#include <vector>
#include <unordered_map>

#include "resources.h"

enum BuildableType {
    UNIT = 0,
    BUILDING = 1
};

class Buildable {
    public:
    virtual BuildableType getType() = 0;
};

// Every buildable thing in the game, with a STABLE number. These cross boundaries -- a
// CommandOrder names what to build by id, and an id is what a savegame or a remote client
// would carry -- so the values are written out explicitly and must never be reused or
// renumbered. Add new ones at the end.
//
// The name is not an identity: it is display text that can change for presentation reasons
// (and two factories could reasonably share one). The id is the identity.
enum BuildableId {
    BUILDABLE_NONE         = 0,

    // Units
    BUILDABLE_SETTLER      = 1,
    BUILDABLE_WORKER       = 2,
    BUILDABLE_WARRIOR      = 3,
    BUILDABLE_SCOUT        = 4,
    BUILDABLE_ARCHER       = 5,
    BUILDABLE_SPEARMAN     = 6,
    BUILDABLE_SWORDMAN     = 7,
    BUILDABLE_AXEMAN       = 8,
    BUILDABLE_PRETORIAN    = 9,
    BUILDABLE_HORSEMAN     = 10,
    BUILDABLE_HORSEARCHER  = 11,
    BUILDABLE_CHARIOT      = 12,
    BUILDABLE_WARELEPHANT  = 13,
    BUILDABLE_WAGON        = 14,
    BUILDABLE_TRIREME      = 15,
    BUILDABLE_GALLEY       = 16,
    BUILDABLE_GALLEON      = 17,
    BUILDABLE_SPY          = 18,

    // Buildings
    BUILDABLE_PALACE       = 40,
    BUILDABLE_BARRACKS     = 41,
    BUILDABLE_GRANARY      = 42,
    BUILDABLE_MARKET       = 43,
    BUILDABLE_COLLOSSEUM   = 44,
    BUILDABLE_WAREHOUSE    = 45,
    BUILDABLE_DEPOT        = 46,
    BUILDABLE_FACTORY      = 47,
    BUILDABLE_OBSERVATORY  = 48
};

class BuildableFactory {
public:
    char name[256];
    // The BuildableId this factory produces. Set by each factory's own constructor -- a
    // factory knows what it is, rather than being told by whatever list created it, so one
    // constructed anywhere (engine.cpp's registry, savegame.cpp, a test) is still identifiable.
    virtual int getId() {
        return id;
    }
    virtual std::vector<int> getRequiredResources() = 0;
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources) = 0;
    virtual Buildable* create() = 0;
    virtual std::vector<int> getDependencyCodes() {
        return dependencyCodes;
    }
protected:
    int id = BUILDABLE_NONE;
    std::vector<int> dependencyCodes;
    void addDependencyCode(int codeId) {
        dependencyCodes.push_back(codeId);
    }
};





#endif // BUILDABLE_H