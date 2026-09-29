#ifndef FACTION_H
#define FACTION_H

#include <vector>
#include <unordered_map>
#include <algorithm>

#include "resources.h"    // FUNDAMENTAL_RATES

class Faction {
    protected:
        bool doneThisTurn;
    public:
        int pop;
        int coins;
        char name[256];
        int id = -1;
        int definition = -1;   // Row of the civilization table this faction was made from (name, colour, song).
        int red, green, blue;

        bool autoPlayer = true; //@NOTE Change this if you want to be able to control the other factions.

        int mapoffset;
        int vmapoffset;

        int blinkingrate = 70;

        int p=0;

        float rates[FUNDAMENTAL_RATES];

        void (*song)() = nullptr; // Pointer to a function that plays the faction's song.

    Faction ()
    {
        doneThisTurn = false;
        mapoffset = vmapoffset=0;
    };

    void done()
    {
        doneThisTurn = true;
    };

    bool isDone()
    {
        return doneThisTurn;
    };

    void ready()
    {
        doneThisTurn = false;
        p=0;
    };
};

// The live civilizations. A hashtable id -> Faction for lookups, plus a vector of the ids in
// the order they arose, which is the turn order and what every loop iterates.
//
// Ids are continuous and never reused: push_back() gives the new faction the next id and adds
// it at the end, erase() drops a lost one. A faction id is therefore not tied to a
// civilization -- a civilization that is lost and returns comes back under a new id, so the
// per-id tables (diplomacy, tech tree, fog of war) never hand it somebody else's leftovers.
class Factions
{
    public:
        // Assigns f->id (the next id) and appends it. Returns the id.
        int push_back(Faction* f)
        {
            f->id = nextid++;
            byid[f->id] = f;
            ids.push_back(f->id);
            return f->id;
        }

        // A faction coming back from a savegame: keeps the id it has. False for an id already
        // in use.
        bool restore(Faction* f)
        {
            if (has(f->id))
                return false;
            byid[f->id] = f;
            ids.push_back(f->id);
            if (f->id >= nextid)
                nextid = f->id + 1;
            return true;
        }

        // A savegame also restores where the ids go on: past every faction ever made, lost
        // ones included.
        void setNextId(int id) { if (id > nextid) nextid = id; }

        // Deletes every faction and starts the ids over (a savegame replaces the list).
        void clear()
        {
            for (auto& [id, f] : byid)
                delete f;
            byid.clear();
            ids.clear();
            nextid = 0;
        }

        // Forgets the faction (the caller owns deleting it).
        void erase(int id)
        {
            byid.erase(id);
            ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
        }

        // nullptr when id is not a live faction.
        Faction* operator[](int id) const
        {
            auto it = byid.find(id);
            return it == byid.end() ? nullptr : it->second;
        }

        bool has(int id) const { return byid.find(id) != byid.end(); }

        // Live factions.
        size_t size() const { return ids.size(); }

        // The id the next faction will get: one past the highest id ever given.
        int nextId() const { return nextid; }

        // Turn order: the first live faction, and the one after `id` (-1 when there is none).
        int first() const { return ids.empty() ? -1 : ids.front(); }
        int next(int id) const
        {
            auto it = std::find(ids.begin(), ids.end(), id);
            if (it == ids.end() || ++it == ids.end()) return -1;
            return *it;
        }

        struct iterator
        {
            std::vector<int>::const_iterator it;
            const Factions* owner;
            Faction* const& operator*() const { return owner->byid.at(*it); }
            iterator& operator++() { ++it; return *this; }
            bool operator!=(const iterator& o) const { return it != o.it; }
        };
        iterator begin() const { return iterator{ids.begin(), this}; }
        iterator end() const { return iterator{ids.end(), this}; }

    private:
        std::unordered_map<int, Faction*> byid;
        std::vector<int> ids;
        int nextid = 0;
};

#endif // FACTION_H
