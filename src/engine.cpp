#include "Faction.h"
#include "gamekernel.h"
#include "usercontrols.h"
#include "map.h"
#include "cityscreenui.h"
#include "City.h"
#include "resources.h"

#include "units/Unit.h"
#include "units/Warrior.h"
#include "units/Settler.h"

extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;

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

int nextMovableUnitId(int faction, int currentid)
{
    // @FIXME: Implement a circular queue to manage this
    int id = 0;
    for (auto& [k, c] : units) 
    {
        if (c->faction==faction && c->availablemoves>0 && (currentid != c->id)) 
        {
            id = c->id;
            return id;
        }
    }
    return id;
}


