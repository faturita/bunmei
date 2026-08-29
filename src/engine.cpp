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
#include "units/Wagon.h"
#include "coordinator.h"
#include "dee.h"
#include "messages.h"
#include "sounds/sounds.h"
#include "engine.h"

extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Coordinator coordinator;
extern DependencyEvaluationEngine dee;
extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern ImprovementEffort improvementeffort;
extern ImprovementResources improvementresources;
extern ImprovementBiomaRestrictions improvementbiomarestrictions;
extern MovementCost movementcosts;

extern DiplomacyTable diplomacy;

extern int year;

// Cost in movement points of moving from a tile onto an adjacent one (real coordinates):
// the bioma of the DESTINATION tile decides (initMovementCosts), unless the two tiles are
// connected by a road or a railroad, which override the terrain cost.
float travelCost(int fromlat, int fromlon, int tolat, int tolon)
{
    mapcell &from = map.peek(fromlat,fromlon);
    mapcell &to   = map.peek(tolat,tolon);

    if (from.hasRailroad() && to.hasRailroad())
        return RAILROAD_MOVEMENT_COST;

    if ((from.hasRoad() || from.hasRailroad()) && (to.hasRoad() || to.hasRailroad()))
        return ROAD_MOVEMENT_COST;

    // The bioma variants (grassland_w, ...) share the cost of their base bioma (high nibble).
    int basebioma = to.bioma & 0xf0;
    if (movementcosts.find(basebioma) != movementcosts.end())
        return movementcosts[basebioma];

    return 1.0f;
}

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

// Go through all the things a city can build and check all the dependencies.
void populateCityBuildables(City* city)
{
    static std::vector<BuildableFactory*> buildable; // Add all the buildables things of the game.
    if (buildable.empty())
    {
        buildable.push_back(new BarracksFactory());
        buildable.push_back(new PalaceFactory());
        buildable.push_back(new ScoutFactory());
        buildable.push_back(new SettlerFactory());
        buildable.push_back(new WorkerFactory());
        buildable.push_back(new GranaryFactory());
        buildable.push_back(new WagonFactory());
        buildable.push_back(new CollosseumFactory());
        buildable.push_back(new MarketFactory());
        buildable.push_back(new WarriorFactory());
        buildable.push_back(new ArcherFactory());
        buildable.push_back(new SpearmanFactory());
        buildable.push_back(new SwordmanFactory());
        buildable.push_back(new PretorianFactory());
        buildable.push_back(new AxemanFactory());
        buildable.push_back(new HorsemanFactory());
        buildable.push_back(new ChariotFactory());
        buildable.push_back(new WarelephantFactory());
        buildable.push_back(new TriremeFactory());
        buildable.push_back(new GalleyFactory());
        buildable.push_back(new HorsearcherFactory());
    }


    for (auto& buildableFactory : buildable)
    {
        if (!dee.verifyDepAll(factionContext(city->faction), buildableFactory->getDependencyCodes()))
            continue;

        // Buildings can only be built once per city (units have no such limit, and
        // city->buildings only ever holds Buildings, never Units, so this check is a
        // no-op for unit factories). Factory and instance share the same name (e.g.
        // GranaryFactory/Granary are both "Granary"), so a name match is enough to tell
        // a building has already been built here.
        bool alreadyBuilt = false;
        for (auto& b : city->buildings)
            if (strcmp(b->name, buildableFactory->name) == 0)
            {
                alreadyBuilt = true;
                break;
            }
        if (alreadyBuilt)
            continue;

        city->buildable.push_back(buildableFactory);
    }

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

    //printf("Setting up faction %d - %s\n",coordinator.a_f_id,factions[coordinator.a_f_id]->name);
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


bool attack(Unit* attacker, int lat, int lon)
{
    std::vector<int> unitstodelete;
    bool confirmed = false;

    // Attacking requires landSeizure with the defender's faction (README.md DefCon table):
    // an open-borders-only relation (trade agreement, coalition, vassalage) lets you walk in
    // but not fight.
    bool hostile = !map.set(lat,lon).isFreeLand() && !map.set(lat,lon).isOwnedBy(attacker->faction) &&
                   diplomacy[attacker->faction][map.set(lat,lon).getOwnedBy()].landSeizure;

    if (hostile)
    {
        // Find the enemy unit located there
        Unit *defender = nullptr;

        City* city = findCityAt(lat,lon);

        int numberofdefenders = 0;
        defender = getDefender(lat,lon,numberofdefenders,attacker->faction);

        Unit *winner = nullptr;
        Unit *loser = nullptr;

        //assert(defender!=nullptr || !"Error: a tile is marked by owner but it does not belong to a city and there aren't any unit in it.");
        if (defender!=nullptr)
        {
            int chance = getRandomInteger(0,1);

            // Coordinate who wins the battle.
            if (defender->getDefense()>attacker->getAttack() || (defender->getDefense()==attacker->getAttack() && chance == 0) )
            {
                lose();
                winner = defender;
                loser = attacker;
            }
            else if (defender->getDefense()<attacker->getAttack() || (defender->getDefense()==attacker->getAttack() && chance == 1))
            {
                win();
                winner = attacker;
                loser = defender;
            }

            // @NOTE: Eventually we can have a draw, a stalemate, or a retreat.
        }
        else
        {
            return false;
        }

        if (winner == attacker && city == nullptr && numberofdefenders==1)
        {
            // The attacker wins, move forward capturing the new tile.
            map.set(attacker->latitude, attacker->longitude).releaseOwner();

            // Confirm the change
            attacker->update(lat,lon);

            map.set(attacker->latitude, attacker->longitude).setOwnedBy(attacker->faction);

            attacker->availablemoves=0;

            loser->destroy();
            confirmed = true;
        }
        else
        if (winner == attacker && (city != nullptr || numberofdefenders>1) )
        {
            // Move forward, do not confirm it and go back.
            attacker->update(lat,lon);

            attacker->availablemoves--;

            attacker->goBackOnCompletion();

            loser->destroy();
            confirmed = true;
        } else
        if (winner == defender)
        {
            map.set(attacker->latitude, attacker->longitude).releaseOwner();

            attacker->availablemoves=0;

            attacker->update(lat,lon);
            coordinator.a_u_id = nextMovableUnitId(coordinator.a_f_id);

            attacker->markForDeletion();
            confirmed = true;
        }


    }   

    return confirmed;
}


bool captureCity(Unit* invader, int lat, int lon)
{
    // Move into an empty city.
    if (map.set(lat,lon).belongsToCity())
    {
        // Find the city located there
        City *city = findCityAt(lat,lon);

        if (city!=nullptr)
        {
            // Capturing requires landSeizure with the city's faction (README.md DefCon
            // table), same as attack().
            bool hostile = city->faction != invader->faction &&
                           diplomacy[invader->faction][city->faction].landSeizure;

            // Check if the city is not defended.
            if (hostile && !city->isDefendedCity())
            {

                map.set(invader->latitude, invader->longitude).releaseOwner();

                invader->update(lat,lon);

                map.set(invader->latitude, invader->longitude).setOwnedBy(invader->faction);

                invader->availablemoves=0;   

                // Perhaps we should do some form of cleaning first, and a reassignment.
                city->reAssignWorkingTiles(invader->faction);
                city->faction = invader->faction;
                city->setDefense();

                // Units caught inside the city could not defend it (or it would not have been
                // captured): they are captured too and flip to the conquering faction.
                for (auto& [k, u] : units)
                {
                    if (u->latitude == lat && u->longitude == lon && u->faction != invader->faction)
                    {
                        u->faction = invader->faction;
                        u->availablemoves = 0;
                        message(year, invader->faction, "A %s in %s has been captured by %s.", u->name, city->name, factions[invader->faction]->name);
                    }
                }

                march();
                message(year, invader->faction, "City %s has been conquered by %s. %d pieces plundered.",city->name, factions[invader->faction]->name, city->resources[COINS]);  

                // @FIXME: We may loose some coins here.  I am just capturing everything.
                printf("Capture City Condition\n");
                return true;    
            }
        }
    }   

    return false; 
}


bool moveForward(Unit* unit, int lat, int lon)
{
    // @FIXME: I am checking consistency again here...
    if (!((map.set(lat,lon).code==LAND && unit->getMovementType()==LANDTYPE) || 
        (map.set(lat,lon).code==OCEAN && unit->getMovementType()==OCEANTYPE) ))
    {
        return false;
    }
 

    LandEntry entry = evaluateLandEntry(unit->faction, map.set(lat,lon));

    if (entry == LandEntry::BLOCKED)
    {
        if (!factions[coordinator.a_f_id]->autoPlayer)
            blocked();
        return false;
    }

    // March into a new tile (only allows movement in the tiles that I own @FIXME)
    {
        float cost = travelCost(unit->latitude, unit->longitude, lat, lon);

        if (cost > unit->availablemoves)
        {
            // The tile costs more than the unit has: the unit stays, goes into movement
            // DEBT (availablemoves negative) and the move completes at the endOfYear
            // refresh once availablemoves recovers to >= 0.
            unit->availablemoves -= cost;
            unit->setPendingMove(coordinate(lat,lon));

            printf("Pending move condition: cost %.2f, moves left %.2f\n", cost, unit->availablemoves);
            return true;
        }

        map.set(unit->latitude, unit->longitude).releaseOwner();

        // Normal, regular movement....
        unit->update(lat,lon);

        unit->availablemoves -= cost;

        if (entry == LandEntry::ENTER_AND_CLAIM)
            map.set(unit->latitude, unit->longitude).setOwnedBy(unit->faction);

        printf("Move forward condition\n");
        return true;

    }


}

bool moveOntoNavalUnit(Unit* passenger, Trireme* navalunit, int lat, int lon)
{
    if (navalunit!=nullptr)
    {
        if (navalunit->board(passenger))
        {
            map.set(passenger->latitude, passenger->longitude).releaseOwner();

            passenger->update(lat,lon);
            passenger->sentry();

            // @FIXME: Check what is the meaning of this here....
            //map.set(passenger->latitude, passenger->longitude).setOwnedBy(passenger->faction);

            passenger->availablemoves=0;

            printf("Move onto naval unit condition\n");
            return true;
        } 
        else
        {
            printf("The boat is full.\n");
            return false;
        }
    }

    return false;
}

bool land(Unit* navalunit, int lat, int lon)
{
    // Chek if navalunit is actually a boat, and that we are moving towards a place where is land.
    if (map.set(lat,lon).code == LAND)
    {
        if(Trireme* trireme = dynamic_cast<Trireme*>(units[coordinator.a_u_id]))
        {
            // @FIXME: Check that there are no enemy units and that there are cities and there are no places controlled by cities.
            if (!map.set(lat,lon).isFreeLand())
                return false;

            if (trireme->manifest()>0)
            {
                Unit* passenger = trireme->unboard();

                //map.set(passenger->latitude, passenger->longitude).releaseOwner();

                passenger->wakeUp();
                passenger->update(lat,lon);

                map.set(passenger->latitude, passenger->longitude).setOwnedBy(passenger->faction);

                passenger->availablemoves=0;

                printf("Units Landed condition\n");
                return true;
            }
        }
    }

    return false;

}

// A naval unit entering a city of its OWN faction: the ship docks on the city tile and
// everything it is shipping is unboarded and awakened (enemy cities go through captureCity).
bool dockInCity(Unit* navalunit, int lat, int lon)
{
    if (navalunit->getMovementType()!=OCEANTYPE || map.set(lat,lon).code != LAND || !map.set(lat,lon).belongsToCity())
        return false;

    City* city = findCityAt(lat,lon);

    if (city==nullptr || city->faction != navalunit->faction)
        return false;

    if (Trireme* trireme = dynamic_cast<Trireme*>(navalunit))
    {
        map.set(trireme->latitude, trireme->longitude).releaseOwner();

        // Trireme::update also moves the passengers onto the city tile.
        trireme->update(lat,lon);

        map.set(trireme->latitude, trireme->longitude).setOwnedBy(trireme->faction);

        trireme->availablemoves--;

        while (trireme->manifest()>0)
        {
            Unit* passenger = trireme->unboard();

            passenger->wakeUp();

            map.set(passenger->latitude, passenger->longitude).setOwnedBy(passenger->faction);

            passenger->availablemoves=0;
        }

        printf("Dock in city condition\n");
        return true;
    }

    return false;
}

Trireme* findNavalUnit(int lat, int lon)
{
    Trireme* navalunit = nullptr;
    for(auto& [k,u]:units)
    {
        if (u->getMovementType()==OCEANTYPE && u->latitude == lat && u->longitude == lon)
        {
            navalunit = dynamic_cast<Trireme*>(u);
        }
    }   
    return navalunit; 
}


// Lat, lon are expressed in real map units.
void moveUnit(Unit* unit, int lat, int lon)
{
    if (unit->availablemoves>0)
    {

        // Find a naval unit in the target tile.
        Trireme* navalunit = findNavalUnit(lat,lon);


        // @NOTE: moving into a ship
        if ((map.set(lat,lon).code==LAND && unit->getMovementType()==LANDTYPE) || 
            (map.set(lat,lon).code==OCEAN && navalunit!=nullptr) ||
            (map.set(lat,lon).code==OCEAN && unit->getMovementType()==OCEANTYPE) || 
            (map.set(lat,lon).code==LAND && unit->getMovementType()==OCEANTYPE)) // Allow ocean units to land
        {

            if (!land(unit,lat,lon) && !dockInCity(unit,lat,lon) && !moveOntoNavalUnit(unit, navalunit,lat,lon) && !captureCity(unit,lat,lon) && !attack(unit,lat,lon) && !moveForward(unit,lat,lon))
            {
                // moveForward already shows blocked() itself when diplomacy is what stopped
                // it (see evaluateLandEntry); nothing further to do here.
            }

        } else
        {
            factions[coordinator.a_f_id]->blinkingrate = 10;
            if (!factions[coordinator.a_f_id]->autoPlayer) blocked();  // @FIXME: differentiate between controlling unit and activeunit (active is what i am currently using indeed)
        }
    }
}

void switchUnitIfNoMovesLeft()
{
    if (coordinator.a_u_id != CONTROLLING_NONE)
        if (units.find(coordinator.a_u_id)!=units.end())
            if (units[coordinator.a_u_id]->availablemoves<=0)   // <=: movement debt is negative
            {
                int cid = nextMovableUnitId(coordinator.a_f_id);
                if (cid != CONTROLLING_NONE)
                {
                    coordinator.a_u_id = cid;
                }
                else
                {
                    coordinator.endofturn = true;
                }
            }
}

// Irrigation must sit next to a water source: a RIVER tile (any form -- the base RIVER
// bioma on land, or one of its RIVER_MOUTH_* estuary variants on the ocean tile it flows
// into), a tile with the OASIS special resource, a landlocked ocean (the LAKE bioma,
// tagged by findOceanBodies() at map generation time, gamekernel.cpp), OR a neighbouring
// tile that already has irrigation -- irrigation forms a NETWORK that can be extended
// tile-by-tile away from the original water source with no distance limit, same as the
// classic Civilization mechanic (@Issue: a worker next to an already-irrigated tile was
// being rejected because that tile itself isn't a water/oasis/lake tile). A plain open-ocean
// coastal neighbour does NOT count. Checked on the 4 orthogonal neighbours only (N/S/E/W),
// per the task.
bool tileHasWaterOasisOrIrrigationNearby(int lat, int lon)
{
    mapcell neighbours[4] = { map.north(lat,lon), map.south(lat,lon), map.east(lat,lon), map.west(lat,lon) };

    for (auto &n : neighbours)
    {
        if ((n.bioma & 0xf0) == RIVER) return true;
        if (n.bioma == RIVER_MOUTH_W || n.bioma == RIVER_MOUTH_S || n.bioma == RIVER_MOUTH_E || n.bioma == RIVER_MOUTH_N) return true;
        if (n.bioma == LAKE) return true;
        if (n.resource == OASIS) return true;
        if (n.hasIrrigation()) return true;
    }

    return false;
}

void processCommandOrders()
{
  while (!coordinator.empty())
  {
    CommandOrder co = coordinator.pop();

    // Finalize commands apply to a TILE (carried in co.parameters), not the active unit:
    // by the time processWork() pushes one, the working unit's moves are already zeroed
    // and coordinator.a_u_id may already have moved on (or hit CONTROLLING_NONE, if it was
    // the faction's last movable unit), so these must run before the active-unit guard below.
    if (co.command == Command::MoveUnitTo)
    {
        if (units.find(co.parameters.spawnid) != units.end())
        {
            printf("Lat %d Lon %d  -> (%d,%d) Land %d  Bioma  %x  \n",units[co.parameters.spawnid]->latitude,units[co.parameters.spawnid]->longitude, co.parameters.latitude,co.parameters.longitude, map.set(co.parameters.latitude,co.parameters.longitude).code, map.set(co.parameters.latitude,co.parameters.longitude).bioma);

            // Now move the unit if it is possible.
            moveUnit(units[co.parameters.spawnid],co.parameters.latitude,co.parameters.longitude);

            switchUnitIfNoMovesLeft();
        }
    }

    if (co.command == Command::BuildRoad)
    {
        map.set(co.parameters.latitude, co.parameters.longitude).buildRoad();
        continue;
    }
    if (co.command == Command::BuildMine)
    {
        map.set(co.parameters.latitude, co.parameters.longitude).buildMine();
        continue;
    }
    if (co.command == Command::BuildIrrigation)
    {
        map.set(co.parameters.latitude, co.parameters.longitude).buildIrrigation();
        continue;
    }
    if (co.command == Command::BuildRailroad)
    {
        map.set(co.parameters.latitude, co.parameters.longitude).buildRailroad();
        continue;
    }
    if (co.command == Command::BuildQuarry)
    {
        map.set(co.parameters.latitude, co.parameters.longitude).buildQuarry();
        continue;
    }
    if (co.command == Command::BuildCamp)
    {
        map.set(co.parameters.latitude, co.parameters.longitude).buildCamp();
        continue;
    }
    if (co.command == Command::BuildDerrick)
    {
        map.set(co.parameters.latitude, co.parameters.longitude).buildDerrick();
        continue;
    }
    if (co.command == Command::BuildPlantation)
    {
        map.set(co.parameters.latitude, co.parameters.longitude).buildPlantation();
        continue;
    }
    if (co.command == Command::AssignWorkTileOrder)
    {
        // Addresses a CITY (parameters.cityid), not the active unit, same as the tile
        // commands above -- must run before the active-unit guard below.
        auto cityIt = cities.find(co.parameters.cityid);
        if (cityIt != cities.end())
            cityIt->second->assignWorkingTile(coordinate(co.parameters.latitude, co.parameters.longitude));
        continue;
    }
    if (co.command == Command::PopulateBuildableOrder)
    {
        // Addresses a CITY (parameters.cityid), not the active unit, same as the tile
        // commands above -- must run before the active-unit guard below.
        auto cityIt = cities.find(co.parameters.cityid);
        if (cityIt != cities.end())
        {
            cityIt->second->buildable.clear(); //@TODO: Add all the checks to see what can be built based on science and resources.
            populateCityBuildables(cityIt->second);
        }
        continue;
    }

    if (units.find(co.parameters.spawnid) == units.end())
    {
        continue;
    }

    if (co.command == Command::BuildCityOrder)
    {
        // You cannot build a city in a land CLAIMED already by another city.
        if (!map.set(units[co.parameters.spawnid]->latitude,units[co.parameters.spawnid]->longitude).isUnassignedLand())
        {
            message(year, co.parameters.factionid, "City cannot be built here.  The land is already claimed by another city.");
            return;
        }


        City *city = new City(&map, units[co.parameters.spawnid]->faction,getNextCityId(),units[co.parameters.spawnid]->latitude,units[co.parameters.spawnid]->longitude);
        city->setName(citynames[co.parameters.factionid].front().c_str());
        citynames[co.parameters.factionid].pop();

        // @NOTE: When the population is zero, the first city is the capital city.
        if (factions[co.parameters.factionid]->pop==0)
        {
            city->setCapitalCity();
            // Buildings already built in the city
            city->buildings.push_back(new Palace());

            // @FIXME: This is the momento to make the song of the faction.
            //russians();

        }

        city->foundedyear = year;

        // city->buildable starts empty: it is only filled the first time the player opens
        // the city's Change screen (see cityscreenui.cpp's changeIsActive, which pushes
        // Command::PopulateBuildableOrder).

        // We add the Warrior as the first thing to build in the city.
        city->productionQueue.push(new WarriorFactory());


        cities[city->id] = city;

        // @FIXME: Disband the settler unit.
        Unit *settler = units[co.parameters.spawnid];
        map.set(settler->latitude,settler->longitude).releaseOwner();
        units.erase(co.parameters.spawnid);
        delete settler;

        coordinator.a_u_id = nextMovableUnitId(co.parameters.factionid);

        message(year, co.parameters.factionid, "City %s %shas been founded.",city->name, city->isCapitalCity()?"(Capital) ":"");


    }
    else if (co.command == Command::DisbandUnitOrder)
    {
        Unit *unit = units[co.parameters.spawnid];
        map.set(unit->latitude,unit->longitude).releaseOwner();
        units.erase(co.parameters.spawnid);
        delete unit;

        coordinator.a_u_id = nextMovableUnitId(co.parameters.factionid);  //@FIXME: There could be the case that there are no more units.
    }
    else if (co.command == Command::FortifyUnitOrder)
    {
        Unit *unit = units[co.parameters.spawnid];
        unit->fortify();
        unit->availablemoves = 0;

        coordinator.a_u_id = nextMovableUnitId(co.parameters.factionid);
    }
    else if (co.command == Command::SentryUnitOrder)
    {
        Unit *unit = units[co.parameters.spawnid];
        unit->sentry();
        unit->availablemoves = 0;

        coordinator.a_u_id = nextMovableUnitId(co.parameters.factionid);
    } else if (co.command == Command::BuildRoadOrder)
    {
        if(Worker* worker = dynamic_cast<Worker*>(units[co.parameters.spawnid]))
        {
            if (map.set(worker->latitude,worker->longitude).hasRoad())
            {
                message(year, co.parameters.factionid, "Cannot build a road here: already built.");
            }
            else
            {
                int effort = getImprovementEffort(improvementeffort, ROAD, map.set(worker->latitude,worker->longitude).bioma);
                worker->roading(effort);
                worker->availablemoves = 0;

                coordinator.a_u_id = nextMovableUnitId(co.parameters.factionid);
            }
        }
    } else if (co.command == Command::BuildIrrigationOrder)
    {
        if(Worker* worker = dynamic_cast<Worker*>(units[co.parameters.spawnid]))
        {
            if (map.set(worker->latitude,worker->longitude).hasIrrigation())
            {
                message(year, co.parameters.factionid, "Cannot build an irrigation here: already irrigated.");
            }
            else if (!tileBiomaAllowsImprovement(improvementbiomarestrictions, IRRIGATION, map.set(worker->latitude,worker->longitude).bioma))
            {
                message(year, co.parameters.factionid, "Cannot build an irrigation here: unsuitable terrain.");
            }
            else if (!tileHasWaterOasisOrIrrigationNearby(worker->latitude, worker->longitude))
            {
                message(year, co.parameters.factionid, "Cannot build an irrigation here: no river, oasis, lake or irrigated tile nearby.");
            }
            else
            {
                int effort = getImprovementEffort(improvementeffort, IRRIGATION, map.set(worker->latitude,worker->longitude).bioma);
                worker->irrigating(effort);
                worker->availablemoves = 0;

                coordinator.a_u_id = nextMovableUnitId(co.parameters.factionid);
            }
        }
    } else if (co.command == Command::BuildMineOrder)
    {
        if(Worker* worker = dynamic_cast<Worker*>(units[co.parameters.spawnid]))
        {
            if (map.set(worker->latitude,worker->longitude).hasMine())
            {
                message(year, co.parameters.factionid, "Cannot build a mine here: already built.");
            }
            else
            {
                int effort = getImprovementEffort(improvementeffort, MINE, map.set(worker->latitude,worker->longitude).bioma);
                worker->mining(effort);
                worker->availablemoves = 0;

                coordinator.a_u_id = nextMovableUnitId(co.parameters.factionid);
            }
        }
    } else if (co.command == Command::BuildRailroadOrder)
    {
        if(Worker* worker = dynamic_cast<Worker*>(units[co.parameters.spawnid]))
        {
            if (map.set(worker->latitude,worker->longitude).hasRailroad())
            {
                message(year, co.parameters.factionid, "Cannot build a railroad here: already built.");
            }
            else
            {
                int effort = getImprovementEffort(improvementeffort, RAILROAD, map.set(worker->latitude,worker->longitude).bioma);
                worker->railroading(effort);
                worker->availablemoves = 0;

                coordinator.a_u_id = nextMovableUnitId(co.parameters.factionid);
            }
        }
    } else if (co.command == Command::BuildQuarryOrder)
    {
        if(Worker* worker = dynamic_cast<Worker*>(units[co.parameters.spawnid]))
        {
            if (map.set(worker->latitude,worker->longitude).hasQuarry())
            {
                message(year, co.parameters.factionid, "Cannot build a quarry here: already built.");
            }
            else if (!tileHasRequiredResource(improvementresources, QUARRY, map.set(worker->latitude,worker->longitude).resource))
            {
                message(year, co.parameters.factionid, "Cannot build a quarry here: no marble.");
            }
            else
            {
                int effort = getImprovementEffort(improvementeffort, QUARRY, map.set(worker->latitude,worker->longitude).bioma);
                worker->quarrying(effort);
                worker->availablemoves = 0;

                coordinator.a_u_id = nextMovableUnitId(co.parameters.factionid);
            }
        }
    } else if (co.command == Command::BuildCampOrder)
    {
        if(Worker* worker = dynamic_cast<Worker*>(units[co.parameters.spawnid]))
        {
            if (map.set(worker->latitude,worker->longitude).hasCamp())
            {
                message(year, co.parameters.factionid, "Cannot build a camp here: already built.");
            }
            else if (!tileHasRequiredResource(improvementresources, CAMP, map.set(worker->latitude,worker->longitude).resource))
            {
                message(year, co.parameters.factionid, "Cannot build a camp here: no doe, game or seal.");
            }
            else
            {
                int effort = getImprovementEffort(improvementeffort, CAMP, map.set(worker->latitude,worker->longitude).bioma);
                worker->camping(effort);
                worker->availablemoves = 0;

                coordinator.a_u_id = nextMovableUnitId(co.parameters.factionid);
            }
        }
    } else if (co.command == Command::BuildDerrickOrder)
    {
        if(Worker* worker = dynamic_cast<Worker*>(units[co.parameters.spawnid]))
        {
            if (map.set(worker->latitude,worker->longitude).hasDerrick())
            {
                message(year, co.parameters.factionid, "Cannot build a derrick here: already built.");
            }
            else if (!tileHasRequiredResource(improvementresources, DERRICK, map.set(worker->latitude,worker->longitude).resource))
            {
                message(year, co.parameters.factionid, "Cannot build a derrick here: no oil.");
            }
            else
            {
                int effort = getImprovementEffort(improvementeffort, DERRICK, map.set(worker->latitude,worker->longitude).bioma);
                worker->derricking(effort);
                worker->availablemoves = 0;

                coordinator.a_u_id = nextMovableUnitId(co.parameters.factionid);
            }
        }
    } else if (co.command == Command::BuildPlantationOrder)
    {
        if(Worker* worker = dynamic_cast<Worker*>(units[co.parameters.spawnid]))
        {
            if (map.set(worker->latitude,worker->longitude).hasPlantation())
            {
                message(year, co.parameters.factionid, "Cannot build a plantation here: already built.");
            }
            else if (!tileHasRequiredResource(improvementresources, PLANTATION, map.set(worker->latitude,worker->longitude).resource))
            {
                message(year, co.parameters.factionid, "Cannot build a plantation here: no grapes, sugar, tobacco or cotton.");
            }
            else
            {
                int effort = getImprovementEffort(improvementeffort, PLANTATION, map.set(worker->latitude,worker->longitude).bioma);
                worker->planting(effort);
                worker->availablemoves = 0;

                coordinator.a_u_id = nextMovableUnitId(co.parameters.factionid);
            }
        }
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
// Which working state (Unit::isRoading() etc.) maps to which finalize command: a table
// instead of a nested ternary so a future improvement (per the user: more are coming)
// only needs one row here.
static const struct { bool (Unit::*isKind)(); Command finalize; } workKinds[] = {
    { &Unit::isRoading,     Command::BuildRoad },
    { &Unit::isMining,      Command::BuildMine },
    { &Unit::isIrrigating,  Command::BuildIrrigation },
    { &Unit::isRailroading, Command::BuildRailroad },
    { &Unit::isQuarrying,   Command::BuildQuarry },
    { &Unit::isCamping,     Command::BuildCamp },
    { &Unit::isDerricking,  Command::BuildDerrick },
    { &Unit::isPlanting,    Command::BuildPlantation },
};

void processWork()
{
    if (units.find(coordinator.a_u_id) == units.end())
        return;

    Unit* unit = units[coordinator.a_u_id];

    if (!unit->isWorking())
        return;

    Command finalize = Command::None;
    for (const auto& kind : workKinds)
    {
        if ((unit->*(kind.isKind))())
        {
            finalize = kind.finalize;
            break;
        }
    }

    unit->work();
    unit->availablemoves = 0;

    if (unit->workCompleted())
    {
        CommandOrder co;
        co.parameters.latitude = unit->latitude;
        co.parameters.longitude = unit->longitude;
        co.command = finalize;
        coordinator.push(co);
    }

    coordinator.a_u_id = nextMovableUnitId(coordinator.a_f_id);
}

