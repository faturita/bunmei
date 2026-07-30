#include <unordered_map>
#include "Faction.h"
#include "gamekernel.h"
#include "usercontrols.h"
#include "map.h"
#include "cityscreenui.h"
#include "City.h"
#include "resources.h"
#include "tiles.h"
#include "diplomacy.h"

#include "Faction.h"
#include "units/Unit.h"
#include "units/Warrior.h"
#include "units/Settler.h"
#include "buildings/Building.h"
#include "buildings/Palace.h"
#include "buildings/Barracks.h"
#include "buildings/Granary.h"
#include "buildings/Collosseum.h"
#include "buildings/Market.h"

#include "units/Unit.h"
#include "units/Settler.h"
#include "units/Warrior.h"
#include "units/Horseman.h"
#include "units/Worker.h"
#include "units/Trireme.h"
#include "units/Archer.h"
#include "units/Swordman.h"
#include "units/Spearman.h"
#include "units/Axeman.h"
#include "units/Horsearcher.h"
#include "units/Galley.h"
#include "units/Scout.h"
#include "units/Warelephant.h"
#include "units/Chariot.h"
#include "units/Pretorian.h"
#include "units/Spy.h"
#include "coordinator.h"
#include "messages.h"

#include "engine.h"

extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Coordinator coordinator;
extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern ImprovementEffort improvementeffort;

extern DiplomacyTable diplomacy;

extern int year;

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
        if (u->faction==f_id && u->availablemoves>0 && u->isSentry()==false && u->isFortified()==false && u->isDying()==false)
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

int findNearbyEnemyFactionId(int unitId, int radius)
{
    // @FIXME: Check if this is really necessary....
    auto it = units.find(unitId);
    if (it == units.end() || it->second == nullptr)
    {
        return -1;
    }

    Unit* u = it->second;
    int targetFactionId = -1;

    int maxDistSq = (radius > 0) ? (radius * radius) : -1;
    int nearestDistSq = -1;

    for (auto& [otherUnitId, otherUnit] : units)
    {
        if (otherUnitId == unitId || otherUnit == nullptr)
        {
            continue;
        }

        if (otherUnit->faction == u->faction)
        {
            continue;
        }

        int dLat = otherUnit->latitude - u->latitude;
        int dLon = otherUnit->longitude - u->longitude;
        int distSq = (dLat * dLat + dLon * dLon);

        if (maxDistSq >= 0 && distSq > maxDistSq)
        {
            continue;
        }

        if (nearestDistSq < 0 || distSq < nearestDistSq)
        {
            nearestDistSq = distSq;
            targetFactionId = otherUnit->faction;
        }
    }

    return targetFactionId;
}

void activateUnit(Unit* u)
{
    coordinator.a_u_id = u->id;

    if (u->isFortified())
        u->packUp();

    if (u->isSentry())
        u->wakeUp();

    if (u->isWorking())
        u->completed();
}

bool noMoreMovementsLeft(int fid)
{
    bool nomore = true;
    for(auto& [k, u] : units) 
    {
        if (u->faction == fid && !u->isFortified() && !u->isSentry() && u->availablemoves>0)
        {
            nomore = false;
        }
    }
    return nomore;
}

void reSetCities()
{
    for(auto& f:factions)
    {
        //printf("Faction %d - %s red %d\n",f->id,factions[f->id]->name,f->red);

        // @NOTE: Pop and coins are reset here and recalculated later.
        f->pop = 0;
        f->coins = 0;
    }

    // Update all the time if the city is or not defended...
    for(auto& [cid,c]:cities)
    {
        factions[c->faction]->pop += c->pop;
        c->noDefense(); // Set the city as defenseless, and then check if there are units defending it.

        for(auto& [k, u] : units)
        {
            // Only units of the city's own faction that can actually fight defend the city:
            // a settler (defense 0) cannot hold a city, it is captured with it.
            if (u->latitude == c->latitude && u->longitude == c->longitude &&
                u->faction == c->faction && u->getDefense() > 0)
            {
                c->setDefense();
                break;
            }
        }

        // @FIXME: This is a workaround
        if (!c->workingOn(0,0))
        {
            map.set(c->latitude+0, c->longitude+0).setCityOwnership(c->faction, c->id);        
        }
        c->deAssigntWorkingTile();


        // @NOTE Collect taxes....
        factions[c->faction]->coins += c->resources[COINS];

        // @FIXME: Spread culture

        // @FIXME: Collect science.

    }    
}

void setUpFaction()
{

    printf("Setting up faction %d - %s\n",coordinator.a_f_id,factions[coordinator.a_f_id]->name);
    coordinator.a_u_id=nextMovableUnitId(coordinator.a_f_id);

    reSetCities();

}

bool endOfTurnForAllFactions()
{
    for(auto& f:factions)
    {
        if (!f->isDone())
            return false;
    }
    return true;
}


LandEntry evaluateLandEntry(int f_id, mapcell &cell)
{
    if (cell.isFreeLand() || cell.isOwnedBy(f_id))
        return LandEntry::ENTER_AND_CLAIM;

    int owner = cell.getOwnedBy();
    if (diplomacy[f_id][owner].landSeizure)
        return LandEntry::ENTER_AND_CLAIM;
    if (diplomacy[f_id][owner].openBorders)
        return LandEntry::ENTER;
    return LandEntry::BLOCKED;
}

// Execute a move that was left pending while the unit paid its movement debt.
// The map may have changed in the meantime, so the move is re-validated; if it is no
// longer possible the pending move is simply cancelled (the unit stays where it is).
void completePendingMove(Unit* unit)
{
    coordinate t = unit->getPendingMove();
    unit->clearPendingMove();

    if (!((map.set(t.lat,t.lon).code==LAND && unit->getMovementType()==LANDTYPE) ||
        (map.set(t.lat,t.lon).code==OCEAN && unit->getMovementType()==OCEANTYPE) ))
        return;

    LandEntry entry = evaluateLandEntry(unit->faction, map.set(t.lat,t.lon));
    if (entry == LandEntry::BLOCKED)
        return;

    // A plain move cannot end on an enemy unit or an enemy city (combat and capture are
    // resolved by moveUnit at the moment the order is given, not here).
    for (auto& [k, u] : units)
        if (u->latitude == t.lat && u->longitude == t.lon && u->faction != unit->faction)
            return;

    for (auto& [k, c] : cities)
        if (c->latitude == t.lat && c->longitude == t.lon && c->faction != unit->faction)
            return;

    map.set(unit->latitude, unit->longitude).releaseOwner();
    unit->update(t.lat,t.lon);

    if (entry == LandEntry::ENTER_AND_CLAIM)
        map.set(unit->latitude, unit->longitude).setOwnedBy(unit->faction);

    printf("Pending move completed: unit %d arrived at (%d,%d)\n", unit->id, t.lat, t.lon);
}

void cleanUnits()
{
    std::vector<int> unitstodelete;

    for(auto& [k, u] : units) 
    {
        if (u->isMarkedForDeletion())
        {
            unitstodelete.push_back(u->id);
        }
    }

    for(auto& uid:unitstodelete)
    {
        Unit* u = units[uid];

        map.set(u->latitude, u->longitude).releaseOwner();

        units.erase(u->id);
        delete u;

        // The active unit can be a dying unit (killed in battle, erased here once its
        // animation completes): the id in the coordinator would go stale and any
        // units[a_u_id] access would insert a null pointer in the map (segfault in drawHUD).
        if (uid == coordinator.a_u_id)
            coordinator.a_u_id = nextMovableUnitId(coordinator.a_f_id);
    }
}

void processCommandOrders()
{
    CommandOrder co = coordinator.pop();  //@FIXME make it a queue.

    // Finalize commands apply to a TILE (carried in co.parameters), not the active unit:
    // by the time processWork() pushes one, the working unit's moves are already zeroed
    // and coordinator.a_u_id may already have moved on (or hit CONTROLLING_NONE, if it was
    // the faction's last movable unit), so these must run before the active-unit guard below.
    if (co.command == Command::BuildRoad)
    {
        map.set(co.parameters.latitude, co.parameters.longitude).buildRoad();
        return;
    }
    if (co.command == Command::BuildMine)
    {
        map.set(co.parameters.latitude, co.parameters.longitude).buildMine();
        return;
    }
    if (co.command == Command::BuildIrrigation)
    {
        map.set(co.parameters.latitude, co.parameters.longitude).buildIrrigation();
        return;
    }
    if (co.command == Command::BuildRailroad)
    {
        map.set(co.parameters.latitude, co.parameters.longitude).buildRailroad();
        return;
    }

    if (units.find(coordinator.a_u_id) == units.end())
    {
        return;
    }

    if (co.command == Command::BuildCityOrder)
    {
        // You cannot build a city in a land CLAIMED already by another city.
        if (!map.set(units[coordinator.a_u_id]->latitude,units[coordinator.a_u_id]->longitude).isUnassignedLand())
        {
            message(year, coordinator.a_f_id, "City cannot be built here.  The land is already claimed by another city.");
            return;
        }


        City *city = new City(&map, units[coordinator.a_u_id]->faction,getNextCityId(),units[coordinator.a_u_id]->latitude,units[coordinator.a_u_id]->longitude);
        city->setName(citynames[coordinator.a_f_id].front().c_str());
        citynames[coordinator.a_f_id].pop();

        // @NOTE: When the population is zero, the first city is the capital city.
        if (factions[coordinator.a_f_id]->pop==0)
        {
            city->setCapitalCity();
            // Buildings already built in the city
            city->buildings.push_back(new Palace());

            // @FIXME: This is the momento to make the song of the faction.
            //russians();

        }

        city->foundedyear = year;

        // What the city can actually build.
        city->buildable.push_back(new BarracksFactory());
        city->buildable.push_back(new PalaceFactory());
        city->buildable.push_back(new ScoutFactory());
        city->buildable.push_back(new SettlerFactory());
        city->buildable.push_back(new WorkerFactory());
        city->buildable.push_back(new GranaryFactory());
        city->buildable.push_back(new CollosseumFactory());
        city->buildable.push_back(new MarketFactory());
        city->buildable.push_back(new WarriorFactory());
        city->buildable.push_back(new ArcherFactory());
        city->buildable.push_back(new SpearmanFactory());
        city->buildable.push_back(new SwordmanFactory());
        city->buildable.push_back(new PretorianFactory());
        city->buildable.push_back(new AxemanFactory());
        city->buildable.push_back(new WorkerFactory());
        city->buildable.push_back(new HorsemanFactory());
        city->buildable.push_back(new ChariotFactory());
        city->buildable.push_back(new WarelephantFactory());
        city->buildable.push_back(new TriremeFactory());
        city->buildable.push_back(new GalleyFactory());
        city->buildable.push_back(new HorsearcherFactory());


        // We add the Warrior as the first thing to build in the city.
        city->productionQueue.push(new WarriorFactory());

        
        cities[city->id] = city;

        // @FIXME: Disband the settler unit.
        Unit *settler = units[coordinator.a_u_id];
        map.set(settler->latitude,settler->longitude).releaseOwner();
        units.erase(coordinator.a_u_id);
        delete settler;

        coordinator.a_u_id = nextMovableUnitId(coordinator.a_f_id);

        message(year, coordinator.a_f_id, "City %s %shas been founded.",city->name, city->isCapitalCity()?"(Capital) ":"");


    }
    else if (co.command == Command::DisbandUnitOrder)
    {
        Unit *unit = units[coordinator.a_u_id];
        map.set(unit->latitude,unit->longitude).releaseOwner();
        units.erase(coordinator.a_u_id);
        delete unit;

        coordinator.a_u_id = nextMovableUnitId(coordinator.a_f_id);  //@FIXME: There could be the case that there are no more units.
    }
    else if (co.command == Command::FortifyUnitOrder)
    {
        Unit *unit = units[coordinator.a_u_id];
        unit->fortify();
        unit->availablemoves = 0;

        coordinator.a_u_id = nextMovableUnitId(coordinator.a_f_id);
    }
    else if (co.command == Command::SentryUnitOrder)
    {
        Unit *unit = units[coordinator.a_u_id];
        unit->sentry();
        unit->availablemoves = 0;

        coordinator.a_u_id = nextMovableUnitId(coordinator.a_f_id);
    } else if (co.command == Command::BuildRoadOrder)
    {
        if(Worker* worker = dynamic_cast<Worker*>(units[coordinator.a_u_id]))
        {
            int effort = getImprovementEffort(improvementeffort, ROAD, map.set(worker->latitude,worker->longitude).bioma);
            worker->roading(effort);
            worker->availablemoves = 0;

            coordinator.a_u_id = nextMovableUnitId(coordinator.a_f_id);
        }
    } else if (co.command == Command::BuildIrrigationOrder)
    {
        if(Worker* worker = dynamic_cast<Worker*>(units[coordinator.a_u_id]))
        {
            int effort = getImprovementEffort(improvementeffort, IRRIGATION, map.set(worker->latitude,worker->longitude).bioma);
            worker->irrigating(effort);
            worker->availablemoves = 0;

            coordinator.a_u_id = nextMovableUnitId(coordinator.a_f_id);
        }
    } else if (co.command == Command::BuildMineOrder)
    {
        if(Worker* worker = dynamic_cast<Worker*>(units[coordinator.a_u_id]))
        {
            int effort = getImprovementEffort(improvementeffort, MINE, map.set(worker->latitude,worker->longitude).bioma);
            worker->mining(effort);
            worker->availablemoves = 0;

            coordinator.a_u_id = nextMovableUnitId(coordinator.a_f_id);
        }
    } else if (co.command == Command::BuildRailroadOrder)
    {
        if(Worker* worker = dynamic_cast<Worker*>(units[coordinator.a_u_id]))
        {
            int effort = getImprovementEffort(improvementeffort, RAILROAD, map.set(worker->latitude,worker->longitude).bioma);
            worker->railroading(effort);
            worker->availablemoves = 0;

            coordinator.a_u_id = nextMovableUnitId(coordinator.a_f_id);
        }
    }
}

// Building a road/mine/irrigation takes several turns (BuildRoadOrder etc. only put the
// worker into the corresponding working state, with the effort looked up from
// improvementeffort).  Every turn the working unit is reactivated, instead of moving it
// spends its moves performing the task: this drives that, mirroring how processGoTo()
// drives an isAuto() unit's movement without further player input.  When the effort
// reaches zero, work() already clears the unit's working flag, so which improvement to
// finalize is captured BEFORE calling it; the finalize command carries the tile's
// coordinates (not coordinator.a_u_id, which may already have moved on by the time
// processCommandOrders handles it).
void processWork()
{
    if (units.find(coordinator.a_u_id) == units.end())
        return;

    Unit* unit = units[coordinator.a_u_id];

    bool wasRoading = unit->isRoading();
    bool wasMining = unit->isMining();
    bool wasIrrigating = unit->isIrrigating();
    bool wasRailroading = unit->isRailroading();

    if (!wasRoading && !wasMining && !wasIrrigating && !wasRailroading)
        return;

    unit->work();
    unit->availablemoves = 0;

    if (unit->workCompleted())
    {
        CommandOrder co;
        co.parameters.latitude = unit->latitude;
        co.parameters.longitude = unit->longitude;
        co.command = wasRoading ? Command::BuildRoad : (wasMining ? Command::BuildMine : (wasIrrigating ? Command::BuildIrrigation : Command::BuildRailroad));
        coordinator.push(co);
    }

    coordinator.a_u_id = nextMovableUnitId(coordinator.a_f_id);
}