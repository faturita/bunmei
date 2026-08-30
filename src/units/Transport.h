#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <iostream>
#include <vector>
#include "../shippable.h"

class Unit;

class Transport // Interface
{
    public:
        virtual bool board(Shippable* passenger) = 0;
        virtual Shippable* unboard() = 0;
        virtual int manifest() = 0;

        // How many cargo slots this Transport has in total (not how many are free).
        virtual int capacity() = 0;

        // Peek (without removing) the boarded item with this Shippable id, or nullptr if
        // nothing aboard matches -- used to stack more of the same resource onto an existing
        // load instead of taking up a second cargo slot.
        virtual Shippable* findCargo(int id) = 0;

        // Snapshot of everything currently aboard, in a stable per-frame order (index i is
        // "cargo slot i" for the city UI's box icons) -- NOT a live reference into the
        // passenger map, so removing an entry elsewhere doesn't invalidate it mid-iteration.
        virtual std::vector<Shippable*> getCargo() = 0;

        // Removes and returns the first boarded passenger that is actually a Unit (skips
        // over resource cargo) -- used when a naval unit disembarks passengers onto land
        // (land()/dockInCity() in engine.cpp), which should never eject cargo overboard.
        // Returns nullptr if nothing aboard is a Unit.
        virtual Unit* unboardUnit() = 0;

        // Removes (but does not delete) the boarded item with this Shippable id, if any --
        // the targeted counterpart to unboard() (which just pops an arbitrary entry), used
        // when unloading one specific resource stack back into a city. Returns whether
        // something was actually removed.
        virtual bool removeCargo(int id) = 0;
};

#endif   // TRANSPORT_H