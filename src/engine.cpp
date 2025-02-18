#include <unordered_map>
#include "Faction.h"
#include "gamekernel.h"
#include "usercontrols.h"
#include "map.h"
#include "cityscreenui.h"
#include "City.h"
#include "resources.h"

#include "Faction.h"
#include "units/Unit.h"
#include "units/Warrior.h"
#include "units/Settler.h"

extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;


int getNextCityId()
{
    int nextid = 0;
    for (auto& [k, c] : cities) 
    {
        if (c->id>nextid) nextid = c->id;
    }
    return nextid+1;
}

int getNextUnitId()
{
    int nextid = 0;
    for (auto& [k, c] : units) 
    {
        if (c->id>nextid) nextid = c->id;
    }
    return nextid+1;
}

int nextUnitId(int faction)
{
    int id = 0;
    for (auto& [k, c] : units) 
    {
        if (c->faction==faction) 
        {
            id = c->id;
            break;
        }
    }
    return id;
}


// This function returns the next unit that can be moved.
int nextMovableUnitId(int f_id)
{
    Faction *faction = factions[f_id];

    std::vector<int> ids;
    for (auto& [k, u] : units) 
    {
        if (u->faction==f_id && u->availablemoves>0 && u->isSentry()==false && u->isFortified()==false) 
        {
            ids.push_back(u->id);
        }
    }

    if (ids.size()==0) return CONTROLLING_NONE;
    return ids[(faction->p)++ % ids.size()];
}

City* findCityAt(int lat, int lon)
{
    City* city = nullptr;
    for (auto& [k, c] : cities) 
    {
        if (c->latitude == lat && c->longitude == lon)
        {
            city = c;
        }
    }    
    return city;
}

Unit* getDefender(int lat, int lon, int &numberofdefenders, int f_id)
{
    Unit* defender = nullptr;
    numberofdefenders = 0;
    for (auto& [k, u] : units) 
    {
        if (u->latitude == lat && u->longitude == lon && u->faction != f_id)
        {
            // @NOTE: How to pick which defender.  This should be rule-based.
            defender = u;
            numberofdefenders++;
        }
    }

    return defender;
}

