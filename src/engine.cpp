#include <unordered_map>
#include <algorithm>
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
#include "units/Ship.h"

#include "units/Warrior.h"
#include "units/Settler.h"
#include "buildings/Building.h"
#include "buildings/Palace.h"
#include "buildings/Barracks.h"
#include "buildings/Granary.h"
#include "buildings/Collosseum.h"
#include "buildings/Market.h"
#include "buildings/Factory.h"
#include "buildings/Observatory.h"

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
#include "units/Galleon.h"



#include "coordinator.h"
#include "dee.h"
#include "technologies.h"
#include "messages.h"
#include "sounds/sounds.h"
#include "engine.h"

extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern Coordinator coordinator;
extern DependencyEvaluationEngine dee;
extern TechTree techtree;
extern Map map;
extern std::unordered_map<int,std::queue<std::string>> citynames;
extern ImprovementEffort improvementeffort;
extern ImprovementResources improvementresources;
extern ImprovementBiomaRestrictions improvementbiomarestrictions;
extern MovementCost movementcosts;
extern Tiles tiles;

extern DiplomacyTable diplomacy;
extern Controller controller;
extern std::unordered_map<int, int> prices;

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

// Gives every tile on the map its base production rates from the productionrates tables
// (tiles.h). Runs on a freshly generated world AND right after a map is loaded (loadMap,
// mapio.cpp, calls it), so a tile's yield never has to be carried in a save file -- which is
// what let the stored, improvement-inflated rates feed back into themselves before.
//
// One copy, deliberately: this used to be duplicated in gamekernel.cpp and simulate.cpp, and
// the two had silently DRIFTED -- simulate.cpp's older hand-written chain gave grassland
// FOOD 3 where the table says 1 and plain land 2 where it says 1, so the simulator and the
// game were modelling different worlds. engine.cpp is linked into every build.
//
// Improvements are not folded in here; getResourceProductionRate() applies them live, so a
// worker finishing a road takes effect with no re-assignment.
void assignProductionRates(Map &mmp)
{
    for(int lat=mmp.minlat;lat<mmp.maxlat;lat++)
        for (int lon=mmp.minlon;lon<mmp.maxlon;lon++)
        {
            mapcell &cell = mmp.set(lat,lon);

            std::array<int,6> rates = tileBaseProductionRates(productionrates, cell.code, cell.bioma, cell.resource);

            // The vector is sized on the first pass and overwritten on any later one, so
            // re-assigning an already-populated map (after a load) is safe and idempotent.
            while (cell.getResourceProductionRateSize() < (int)rates.size())
                cell.addResourceProductionRate(0);

            for (size_t i = 0; i < rates.size(); i++)
                cell.setResourceProductionRate((int)i, rates[i]);
        }
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

// The city that acts as a faction's coin treasury for trade (commerce screen buy/sell):
// its capital, or -- if none is flagged -- the first city of that faction found. nullptr if
// the faction has no cities. The engine keeps no persistent per-faction coin pot
// (Faction::coins is recomputed from city COINS every frame in reSetCities), so trade has
// to move coins through a real city.
City* factionTreasury(int faction_id)
{
    City* firstCity = nullptr;
    for (auto& [k, c] : cities)
    {
        if (c->faction != faction_id) continue;
        if (c->isCapitalCity()) return c;
        if (firstCity == nullptr) firstCity = c;
    }
    return firstCity;
}

// The bulk of a city's COINS, SCIENCE and CULTURE is its TRADE converted by the faction's
// fundamental rates (the /fundamental command), which endOfYear() does once a year: it
// doubles the trade first if the city has the TRADE_SURPLUS perk, then splits it coins /
// science / culture across rates[0..2].
//
// This mirrors that arithmetic so the city screen can show those rows without waiting for
// the year to turn. It is a projection of the CURRENT turn: TRADE never accumulates
// (endOfYear zeroes it after converting), so the figure comes from the production rate
// rather than from resources[TRADE], which is 0 between turns.
//
// It is an ADDITION to getProductionRate(), never a replacement: all three can also come
// straight off the tiles. No bioma yields them (gamekernel.cpp BASE_PRODUCTION_RATE), but
// SPECIAL RESOURCES do -- a worked GOLD tile gives COINS 2 + CULTURE 1 and GEMS gives
// CULTURE 2 (RESOURCE_RATE_OVERRIDE). No resource currently yields SCIENCE, but that is
// just today's table, not a rule: adding one (an archaeological dig, say) is a single
// RESOURCE_RATE_OVERRIDE row and needs no change here or in the city screen, because both
// sum getProductionRate() with this conversion for EVERY core resource. testcase_065 pins
// that, SCIENCE included.
int cityTradeConversionRate(City* city, int r_id)
{
    int rateIndex;
    switch (r_id)
    {
        case COINS:   rateIndex = 0; break;
        case SCIENCE: rateIndex = 1; break;
        case CULTURE: rateIndex = 2; break;
        default: return 0;                  // every other resource comes off the tiles
    }

    int trade = city->getProductionRate(TRADE) - city->getConsumptionRate(TRADE);
    if (trade < 0) trade = 0;

    if (dee.verifyDep(cityContext(city->id), TRADE_SURPLUS_CODE))
        trade *= 2;

    return (int)((float)trade * factions[city->faction]->rates[rateIndex]);
}

// Ask a faction where its SCIENCE should go next. An autoPlayer faction just rolls one of the
// technologies it can currently research; a human one gets the controller.query selector, one
// option per Frontier technology (at the start of a game that is only "Language", so the
// dialog opens with a single option -- it widens as the Frontier does).
//
// Normally a no-op while the faction still has a valid target. `force` asks anyway: every
// discovery widens the Frontier, so the player is re-prompted after one to decide where the
// science goes now. Either way there is nothing to ask once the Frontier is empty.
void chooseResearch(int factionId, bool force)
{
    if (factionId < 0 || factionId >= techtree.factionCount())
        return;
    if (!force && !techtree.needsResearchTarget(factionId))
        return;
    if (techtree.graph(factionId).getFrontier().empty())
        return;

    if (factions[factionId]->autoPlayer)
    {
        techtree.pickRandomTarget(factionId);
        return;
    }

    // Only one modal dialog fits on screen; if something else is already asking, the fallback
    // in endOfYear() picks for this year and the player is asked again next year.
    if (controller.query.active)
        return;

    std::vector<int> choices = techtree.graph(factionId).getFrontierOrdered();
    if (choices.empty())
        return;

    std::vector<std::string> options;
    for (int id : choices)
    {
        const Tech* t = techtree.graph(factionId).getTech(id);
        options.push_back(t != nullptr ? t->name : std::string("?"));
    }

    controller.query.active  = true;
    controller.query.message = "Our scholars await your direction. What shall we study?";
    controller.query.options = options;
    controller.query.selected = [factionId, choices](int i)
    {
        if (i >= 0 && i < (int)choices.size())
            techtree.setResearchTarget(factionId, choices[i]);
    };
}

// Go through all the things a city can build and check all the dependencies.
// Every buildable the game knows, keyed by its BuildableId (buildable.h). Built once, on
// first use: a factory is stateless, so one instance each is all anyone needs -- city->buildable
// holds pointers INTO this registry rather than copies.
//
// This is the single list of what exists. Before it there were two hardcoded lists that had
// already drifted (populateCityBuildables offered 24 of these, savegame.cpp's loadCities
// pushed a different 11), and nothing tied a factory to a stable number at all.
static std::unordered_map<int, BuildableFactory*>& buildableRegistry()
{
    static std::unordered_map<int, BuildableFactory*> registry;

    if (registry.empty())
    {
        BuildableFactory* all[] = {
            // Units
            new SettlerFactory(),   new WorkerFactory(),      new WarriorFactory(),
            new ScoutFactory(),     new ArcherFactory(),      new SpearmanFactory(),
            new SwordmanFactory(),  new AxemanFactory(),      new PretorianFactory(),
            new HorsemanFactory(),  new HorsearcherFactory(), new ChariotFactory(),
            new WarelephantFactory(), new WagonFactory(),     new TriremeFactory(),
            new GalleyFactory(),    new GalleonFactory(),
            // Buildings
            new PalaceFactory(),    new BarracksFactory(),    new GranaryFactory(),
            new MarketFactory(),    new CollosseumFactory(),  new FactoryFactory(),
            new ObservatoryFactory()
        };

        for (BuildableFactory* bf : all)
        {
            // A factory that forgot to set its id in its constructor, or that collides with
            // one already registered, is a build-time mistake -- say so loudly rather than
            // silently shadowing an entry and making a command target the wrong thing.
            if (bf->getId() == BUILDABLE_NONE)
            {
                printf("BUILDABLE REGISTRY: '%s' has no BuildableId -- its constructor must set one.\n", bf->name);
                continue;
            }
            if (registry.find(bf->getId()) != registry.end())
            {
                printf("BUILDABLE REGISTRY: id %d is claimed by both '%s' and '%s'.\n",
                       bf->getId(), registry[bf->getId()]->name, bf->name);
                continue;
            }
            registry[bf->getId()] = bf;
        }
    }

    return registry;
}

BuildableFactory* buildableFactoryById(int id)
{
    auto& registry = buildableRegistry();
    auto it = registry.find(id);
    return it == registry.end() ? nullptr : it->second;
}

std::vector<BuildableFactory*> allBuildableFactories()
{
    std::vector<BuildableFactory*> out;
    for (auto& [id, bf] : buildableRegistry())
        out.push_back(bf);
    return out;
}

void populateCityBuildables(City* city)
{
    std::vector<BuildableFactory*> buildable = allBuildableFactories();

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

void operateCityBuildings(City* c)
{
    for (Building* building : c->buildings)
    {

        for (int r_id : ALL_CORE_RESOURCES)
        {
            int cr = building->getConsumptionRate(r_id);
            if (cr > 0)
            {
                c->resources[r_id] -= cr;   // Cost deduction
            }
        }


        std::vector<int> consumedResources;
        bool enoughResources = true;

        for (int r_id : ALL_COMMODITIES)
        {
            int cr = building->getConsumptionRate(r_id);
            if (cr > 0)
            {
                if (c->resources[r_id] >= cr)
                    consumedResources.push_back(r_id);
                else
                    enoughResources = false;
            }
        }

        if (!enoughResources)
        {
            message(year, c->faction, "City %s does not have enough resources to operate %s.", c->name, building->name);
            continue;
        }

        // Output is gated on the PRODUCTION rate (getConsumptionRate is 0 for a building's
        // outputs -- a Factory consumes iron, produces tools).
        for (int r_id : ALL_MFG_GOODS)
        {
            int pr = building->getProductionRate(r_id);
            if (pr > 0)
                c->resources[r_id] += pr;
        }

        for (int r_id : consumedResources)
            c->resources[r_id] -= building->getConsumptionRate(r_id);
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
        c->deAssignWorkingTile();

        // @NOTE: Faction->coins are DELETED every time so effective coins remain in cities.
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


bool attack(Unit* attacker, int lat, int lon, bool &forceBreak)
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
        if (attacker->getAttack() == 0)
        {
            message(year, attacker->faction, "A %s cannot attack because it has no attack power.", attacker->name);
            forceBreak = true; // Do not keep processing the move, stop here.
            return false;
        }

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


// A Transport (Wagon/Trireme/Galleon) of the human faction moving onto another faction's
// city tile, with that faction at PEACE or better (DiplomaticStatus >= PEACE): opens the
// commerce screen instead of moving/docking/capturing. Same shape as captureCity()/attack()
// in the moveUnit() chain -- returns true when it handled the move. The transport stays
// where it is (a foreign city tile is not ours to stand on) and just spends its move.
bool engageTrade(Unit* unit, int lat, int lon)
{
    if (dynamic_cast<Transport*>(unit) == nullptr)
        return false;
    if (!map.set(lat,lon).belongsToCity())
        return false;

    City* city = findCityAt(lat,lon);
    if (city == nullptr || city->faction == unit->faction)
        return false;

    // AI transports don't pop a UI; leave their move to fail the normal way (a peaceful
    // foreign border is closed unless openBorders, so moveForward just blocks).
    if (factions[unit->faction]->autoPlayer)
        return false;

    if (diplomacy[unit->faction][city->faction].status < PEACE)
        return false;

    unit->availablemoves = 0;

    controller.view = 4;                 // commerce screen (drawScene / processMouse)
    controller.cityid = city->id;
    controller.tradeunitid = unit->id;

    message(year, unit->faction, "A %s opens trade with %s.", unit->name, city->name);
    return true;
}

bool captureCity(Unit* invader, int lat, int lon, bool &forceBreak)
{
    // A naval unit cannot occupy or capture a land city -- it has no way to hold ground.
    // (Ship now shares the Unit/Transport interface, so without this guard an armed
    // Trireme/Galleon moving onto an undefended enemy city tile would "capture" it.)
    // A defended enemy city is still left to attack() below, so ships can bombard.
    if (invader->getMovementType() == OCEANTYPE)
        return false;

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

                if (invader->getAttack() == 0)
                {
                    message(year, invader->faction, "A %s cannot invade because it has no attack power.", invader->name);
                    forceBreak = true; // Do not keep processing the move, stop here.
                    return false;
                }

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

bool moveOntoNavalUnit(Unit* passenger, Ship* navalunit, int lat, int lon)
{
    if (navalunit!=nullptr)
    {
        // A plain (Shippable*) cast here would be a C-style cast between unrelated types
        // (Unit does not inherit Shippable) -- it compiles down to reinterpret_cast, which
        // does NOT adjust for the passenger's actual Unit/Shippable subobject offsets and
        // produces a corrupt pointer at runtime. dynamic_cast is the only safe cross-cast,
        // and it can legitimately fail here (e.g. a Trireme/Galley/Wagon passenger, none of
        // which implement Shippable) -- board() must not be called with a null passenger.
        Shippable* shippablepassenger = dynamic_cast<Shippable*>(passenger);
        if (shippablepassenger!=nullptr && navalunit->board(shippablepassenger))
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
        if(Ship* trireme = dynamic_cast<Ship*>(units[coordinator.a_u_id]))
        {
            // @FIXME: Check that there are no enemy units and that there are cities and there are no places controlled by cities.
            if (!map.set(lat,lon).isFreeLand())
                return false;

            // unboardUnit() (not unboard()) skips over any resource cargo (Commodity/MfgGood)
            // aboard -- only an actual passenger Unit disembarks here; cargo stays aboard
            // until explicitly unloaded through the city UI.
            if (Unit* passenger = trireme->unboardUnit())
            {
                map.set(passenger->latitude, passenger->longitude).releaseOwner();

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

    if (Ship* trireme = dynamic_cast<Ship*>(navalunit))
    {
        map.set(trireme->latitude, trireme->longitude).releaseOwner();

        // Trireme::update also moves the passengers onto the city tile.
        trireme->update(lat,lon);

        map.set(trireme->latitude, trireme->longitude).setOwnedBy(trireme->faction);

        trireme->availablemoves--;

        // unboardUnit() (not unboard()) only ever removes an actual passenger Unit -- any
        // resource cargo (Commodity/MfgGood) stays aboard the docked ship until explicitly
        // unloaded through the city UI, instead of being silently ejected and lost here.
        while (Unit* passenger = trireme->unboardUnit())
        {
            passenger->wakeUp();

            map.set(passenger->latitude, passenger->longitude).setOwnedBy(passenger->faction);

            passenger->availablemoves=0;
        }

        printf("Dock in city condition\n");
        return true;
    }

    return false;
}

Ship* findNavalUnit(int lat, int lon)
{
    Ship* navalunit = nullptr;
    for(auto& [k,u]:units)
    {
        if (u->getMovementType()==OCEANTYPE && u->latitude == lat && u->longitude == lon)
        {
            navalunit = dynamic_cast<Ship*>(u);
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
        Ship* navalunit = findNavalUnit(lat,lon);


        // @NOTE: moving into a ship
        if ((map.set(lat,lon).code==LAND && unit->getMovementType()==LANDTYPE) || 
            (map.set(lat,lon).code==OCEAN && navalunit!=nullptr) ||
            (map.set(lat,lon).code==OCEAN && unit->getMovementType()==OCEANTYPE) || 
            (map.set(lat,lon).code==LAND && unit->getMovementType()==OCEANTYPE)) // Allow ocean units to land
        {
            bool forceBreak = false;
            bool handled =
                land(unit,lat,lon) ||
                dockInCity(unit,lat,lon) ||
                moveOntoNavalUnit(unit, navalunit,lat,lon) ||
                engageTrade(unit,lat,lon) ||
                captureCity(unit,lat,lon, forceBreak) ||
                (!forceBreak && attack(unit,lat,lon, forceBreak)) ||
                (!forceBreak && moveForward(unit,lat,lon));

            if (!handled)
            {
                // @NOTE: Here it means that for some reason the unit cannot move to the target tile.
                printf("Unit %d cannot move to (%d,%d)\n", unit->id, lat, lon);

                // An AUTOMATED unit whose move is impossible -- blocked by an enemy city or
                // unit it cannot fight through (Settlers never attack), diplomacy, etc. --
                // must not keep re-issuing the exact same blocked step every tick (the game
                // then looks frozen). Drop its moves and clear any GoTo so switchUnitIfNoMovesLeft()
                // advances to the next unit and the AI re-plans from scratch next turn.
                if (unit->faction >= 0 && unit->faction < (int)factions.size() &&
                    factions[unit->faction]->autoPlayer)
                {
                    unit->resetGoTo();
                    unit->availablemoves = 0;
                }
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
// Tags one landlocked ocean body as LAKE so irrigation can use it as a water source, and
// returns how many cells it actually changed.
//
// Only PLAIN open water is tagged. An ocean cell carries OCEANBIOMA once the generator's
// "single water bioma" pass has run and 0 only before it, so both mean untouched water; a cell
// already carrying a river mouth (RIVER_MOUTH_*) keeps it and is still a valid water source
// through the RIVER_MOUTH branch of tileHasWaterOasisOrIrrigationNearby below.
//
// This guard used to be `bioma == 0` alone, which is never true -- OCEANBIOMA is assigned
// first -- so no cell was ever tagged and irrigation beside an inner lake was always refused,
// on a freshly generated map as much as a loaded one.
int tagLakeCells(const std::vector<coordinate>& body)
{
    int tagged = 0;
    for (const coordinate& c : body)
    {
        int bioma = map.peek(c.lat,c.lon).bioma;
        if (bioma == 0 || bioma == OCEANBIOMA)
        {
            map.set(c.lat,c.lon).bioma = LAKE;
            tagged++;
        }
    }
    return tagged;
}

bool tileHasWaterOasisOrIrrigationNearby(int lat, int lon)
{
    // peek(), NOT map.north/south/east/west: those go through Map::operator(), which adds the
    // viewing faction's map offset (Faction::mapoffset, shifted with 'f'/'g') because they are
    // meant for SCREEN coordinates -- map.cpp's drawing loops. lat/lon here are the worker's
    // REAL coordinates, so using them read tiles `offset` columns away and irrigation was
    // refused next to a river whenever the player had scrolled the map.
    mapcell neighbours[4] = { map.peek(lat-1,lon), map.peek(lat+1,lon),
                              map.peek(lat,lon+1), map.peek(lat,lon-1) };

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

// A boarded Resource stack of `resourceid` aboard `transport` that still has room (< 100
// units) to top up -- or nullptr if every stack of that type already aboard is full (or
// there is none yet), meaning the caller needs a NEW cargo slot instead. Transport::findCargo
// only ever returns ONE (the first) matching stack, which isn't enough once a Transport can
// carry several separate stacks of the same resource (each capped at 100) -- see
// LoadCargoOrder/BuyResourceOrder below.
static Resource* findToppableCargo(Transport* transport, int resourceid)
{
    for (Shippable* s : transport->getCargo())
    {
        if (s->getId() != resourceid) continue;
        if (Resource* r = dynamic_cast<Resource*>(s))
            if (r->amount < 100)
                return r;
    }
    return nullptr;
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
    if (co.command == Command::AssignTileOrder || co.command == Command::DeAssignTileOrder)
    {
        // Addresses a CITY (parameters.cityid), not the active unit, same as the tile
        // commands above -- must run before the active-unit guard below. Both are no-ops
        // when the outcome they ask for already holds (City::assignTile/deAssignTile), so
        // nothing here needs to know the tile's current state.
        auto cityIt = cities.find(co.parameters.cityid);
        if (cityIt != cities.end())
        {
            coordinate tile(co.parameters.latitude, co.parameters.longitude);
            City* city = cityIt->second;

            bool changed = (co.command == Command::AssignTileOrder)
                         ? city->assignTile(tile)
                         : city->deAssignTile(tile);

            printf("%s tile (%d,%d) of city %s: %s\n",
                   co.command == Command::AssignTileOrder ? "Assign" : "Deassign",
                   tile.lat, tile.lon, city->name, changed ? "done" : "no change");
        }
        continue;
    }
    if (co.command == Command::DeAssignWorkTileOrder)
    {
        // Addresses a CITY (parameters.cityid), not the active unit, same as the tile
        // commands above -- must run before the active-unit guard below. No tile is named:
        // the city itself picks which surplus tile to give up (see the enum comment).
        auto cityIt = cities.find(co.parameters.cityid);
        if (cityIt != cities.end())
            cityIt->second->deAssignWorkingTile();
        continue;
    }
    if (co.command == Command::ChangeProductionOrder)
    {
        // Addresses a CITY (parameters.cityid), not the active unit -- before the unit guard.
        //
        // The city's OWN buildable list is the authority on what it may build: a name that is
        // not in it is refused outright, which is the check that makes a name safe to accept
        // from a caller. Nothing else here trusts the sender.
        auto cityIt = cities.find(co.parameters.cityid);
        if (cityIt == cities.end())
        {
            printf("ChangeProductionOrder: city %d does not exist.\n", co.parameters.cityid);
            continue;
        }

        City* city = cityIt->second;

        // The city's OWN buildable list is the authority: an id the registry knows but this
        // city cannot currently build (wrong tech, a building it already has) is refused just
        // the same as an id that does not exist at all.
        BuildableFactory* chosen = nullptr;
        for (BuildableFactory* bf : city->buildable)
            if (bf->getId() == co.parameters.selectedbuildableid)
            {
                chosen = bf;
                break;
            }

        if (chosen == nullptr)
        {
            printf("ChangeProductionOrder: %s cannot build buildable id %d.\n",
                   city->name, co.parameters.selectedbuildableid);
            continue;
        }

        // One thing at a time: the queue holds what is being built now, so changing it
        // replaces rather than appends.
        while (!city->productionQueue.empty()) city->productionQueue.pop();
        city->productionQueue.push(chosen);

        message(year, city->faction, "%s is now building %s.", city->name, chosen->name);
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

    if (co.command == Command::SetFundamentalRatesOrder)
    {
        // Addresses a FACTION (parameters.factionid), not the active unit -- so, like the tile
        // and city commands above, it must run BEFORE the active-unit guard below (the
        // /fundamental teletype command has no unit behind it, so spawnid is meaningless).
        // Sets how much of a city's TRADE turns into COINS / SCIENCE / CULTURE / LUXURY each
        // year (bunmei.cpp:endOfYear).
        if (co.parameters.factionid >= 0 && co.parameters.factionid < (int)factions.size())
        {
            Faction* f = factions[co.parameters.factionid];
            for (int i=0;i<4;i++)
                f->rates[i] = co.parameters.rates[i];

            message(year, co.parameters.factionid,
                    "%s fundamental rates set to coins %.2f, science %.2f, culture %.2f, luxury %.2f.",
                    f->name, f->rates[0], f->rates[1], f->rates[2], f->rates[3]);
        }
        continue;
    }

    // A faction id is only usable once it is known to be one. Every faction-addressed
    // handler below re-checks its own inputs even when the pusher already did: the pusher is
    // the local UI today and a remote client tomorrow, and only this side is the authority.
    auto knownFaction = [&](int f) { return f >= 0 && f < (int)factions.size(); };

    if (co.command == Command::SetAutoPlayerOrder)
    {
        // Addresses a FACTION, not the active unit -- before the unit guard below.
        if (knownFaction(co.parameters.factionid))
        {
            Faction* f = factions[co.parameters.factionid];
            f->autoPlayer = co.parameters.enabled;
            message(year, co.parameters.factionid, "%s is now played by %s.",
                    f->name, f->autoPlayer ? "the computer" : "a human");
        }
        else
        {
            printf("SetAutoPlayerOrder: faction %d does not exist.\n", co.parameters.factionid);
        }
        continue;
    }

    if (co.command == Command::SetDiplomacyOrder)
    {
        // Addresses TWO factions. This changes state that belongs to the other faction as
        // much as to the sender, so everything the caller checked is checked again here.
        const int a = co.parameters.factionid;
        const int b = co.parameters.targetfactionid;
        const int status = co.parameters.status;

        if (!knownFaction(a) || !knownFaction(b))
        {
            printf("SetDiplomacyOrder: faction %d or %d does not exist.\n", a, b);
        }
        else if (a == b)
        {
            printf("SetDiplomacyOrder: a faction cannot set a relation with itself (%d).\n", a);
        }
        else if (status < NO_CONTACT || status > VASSALAGE)
        {
            printf("SetDiplomacyOrder: %d is not a diplomatic status.\n", status);
        }
        else
        {
            // The table is undirected: one entry covers both directions, so setting it once
            // is setting it for both factions.
            diplomacy[a][b].setStatus(status);

            char msg[128];
            if (status == PEACE)
                snprintf(msg, sizeof(msg), "%s have declared peace with %s.", factions[a]->name, factions[b]->name);
            else if (status == FOE)
                snprintf(msg, sizeof(msg), "%s are at WAR with %s.", factions[a]->name, factions[b]->name);
            else
                snprintf(msg, sizeof(msg), "%s and %s are now at diplomatic status %d.", factions[a]->name, factions[b]->name, status);

            message(year, a, msg);
            message(year, b, msg);

            if (status == PEACE) peace();
            else if (status == FOE) war();
        }
        continue;
    }

    if (co.command == Command::RegisterDependencyOrder)
    {
        // The command carries a SCOPE, never a pre-encoded dee context id -- the encoding is
        // built here, so a caller cannot hand in a context that means something else.
        if (co.parameters.codeid == 0)
        {
            printf("RegisterDependencyOrder: code 0 registers nothing.\n");
        }
        else if (co.parameters.scope == DEP_SCOPE_WORLD)
        {
            dee.regDep(worldContext(), co.parameters.codeid);
            message(year, co.parameters.factionid, "Enabled code 0x%x for the WORLD.", co.parameters.codeid);
        }
        else if (co.parameters.scope == DEP_SCOPE_FACTION)
        {
            if (knownFaction(co.parameters.factionid))
            {
                dee.regDep(factionContext(co.parameters.factionid), co.parameters.codeid);
                message(year, co.parameters.factionid, "Enabled code 0x%x for faction %s.",
                        co.parameters.codeid, factions[co.parameters.factionid]->name);
            }
            else
            {
                printf("RegisterDependencyOrder: faction %d does not exist.\n", co.parameters.factionid);
            }
        }
        else if (co.parameters.scope == DEP_SCOPE_CITY)
        {
            auto cityIt = cities.find(co.parameters.cityid);
            if (cityIt != cities.end())
            {
                dee.regDep(cityContext(cityIt->second->id), co.parameters.codeid);
                message(year, co.parameters.factionid, "Enabled code 0x%x for city %s.",
                        co.parameters.codeid, cityIt->second->name);
            }
            else
            {
                printf("RegisterDependencyOrder: city %d does not exist.\n", co.parameters.cityid);
            }
        }
        else
        {
            printf("RegisterDependencyOrder: unknown scope %d.\n", co.parameters.scope);
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

        // First city == the first time this faction produces any SCIENCE, so this is where it
        // is asked what to research. Afterwards endOfYear() re-asks whenever the current
        // target drops out of the Frontier.
        chooseResearch(co.parameters.factionid);


    }
    else if (co.command == Command::DisbandUnitOrder)
    {
        Unit *unit = units[co.parameters.spawnid];
        map.set(unit->latitude,unit->longitude).releaseOwner();
        units.erase(co.parameters.spawnid);
        delete unit;

        coordinator.a_u_id = nextMovableUnitId(co.parameters.factionid);  //@FIXME: There could be the case that there are no more units.
    }
    else if (co.command == Command::ActivateUnitOrder)
    {
        // The unit-existence guard above already ran, so it is here rather than with the
        // city/faction commands. What it still has to establish is OWNERSHIP: a unit is only
        // selectable by the faction that owns it, and by a faction whose turn it is.
        Unit *unit = units[co.parameters.spawnid];

        if (unit->faction != co.parameters.factionid)
        {
            printf("Faction %d cannot activate unit %d, which belongs to faction %d.\n",
                   co.parameters.factionid, unit->id, unit->faction);
        }
        else if (unit->availablemoves <= 0 && !unit->isWorking())
        {
            // A working unit's availablemoves is zeroed every turn by processWork(), so it
            // has to stay selectable on that alone -- same as a fortified or sentried one,
            // whose moves also just sit at whatever they were.
            printf("Unit %d has nothing left to do this turn.\n", unit->id);
        }
        else
        {
            activateUnit(unit);
        }
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
    } else if (co.command == Command::LoadCargoOrder)
    {
        // Addresses the active unit (must be a Transport) AND a city (parameters.cityid,
        // where the resource comes from) -- both must resolve, and the unit must actually be
        // docked/stationed there, or this is a no-op.
        Transport* transport = dynamic_cast<Transport*>(units[co.parameters.spawnid]);
        auto cityIt = cities.find(co.parameters.cityid);
        if (transport != nullptr && cityIt != cities.end())
        {
            City* city = cityIt->second;
            int resourceid = co.parameters.resourceid;
            bool ismfggood = resourceid >= rum;   // MFGOODS start at 0x301 (rum), COMMODITIES at 0x201.
            std::unordered_map<int,int>& stockpile = city->resources;

            // A stack of this resource that's not yet at the 100 cap tops up; only when
            // every boarded stack of it is full (or there is none) does this take a new
            // cargo slot -- so a second, third, ... stack of the SAME resource is possible
            // once earlier ones fill up, instead of silently refusing once any exists.
            Resource* existing = findToppableCargo(transport, resourceid);
            if (existing != nullptr)
            {
                int toload = std::min(100 - existing->amount, stockpile[resourceid]);
                if (toload > 0)
                {
                    existing->amount += toload;
                    stockpile[resourceid] -= toload;
                }
            }
            else
            {
                int toload = std::min(100, stockpile[resourceid]);
                if (toload > 0)
                {
                    Resource* cargoitem = ismfggood
                        ? (Resource*) new MfgGood(resourceid, tiles[resourceid].c_str(), "MfgGood")
                        : (Resource*) new Commodity(resourceid, tiles[resourceid].c_str(), "Commodity");
                    cargoitem->amount = toload;

                    if (transport->board(dynamic_cast<Shippable*>(cargoitem)))
                    {
                        stockpile[resourceid] -= toload;
                    }
                    else
                    {
                        delete cargoitem;
                        message(year, co.parameters.factionid, "No room aboard to load more cargo.");
                    }
                }
            }
        }
    } else if (co.command == Command::UnloadCargoOrder)
    {
        Transport* transport = dynamic_cast<Transport*>(units[co.parameters.spawnid]);
        auto cityIt = cities.find(co.parameters.cityid);
        if (transport != nullptr && cityIt != cities.end())
        {
            City* city = cityIt->second;
            Shippable* cargo = transport->findCargo(co.parameters.resourceid);
            if (Resource* r = dynamic_cast<Resource*>(cargo))
            {
                bool ismfggood = co.parameters.resourceid >= rum;
                std::unordered_map<int,int>& stockpile = city->resources;

                stockpile[co.parameters.resourceid] += r->amount;
                transport->removeCargo(co.parameters.resourceid);
                delete r;
            }
        }
    } else if (co.command == Command::BuyResourceOrder)
    {
        // Commerce screen "buy": like LoadCargoOrder, plus the coin transfer. Quantity is
        // one stack (<=100), capped by the city's stock, room aboard, and what the buying
        // faction's treasury can afford at prices[resourceid]. There is no persistent
        // per-faction coin pot (Faction::coins is rebuilt every frame from city COINS in
        // reSetCities), so "the faction pays" == its capital city's resources[COINS] pays.
        Transport* transport = dynamic_cast<Transport*>(units[co.parameters.spawnid]);
        auto cityIt = cities.find(co.parameters.cityid);
        City* treasury = factionTreasury(co.parameters.factionid);
        if (transport != nullptr && cityIt != cities.end() && treasury != nullptr)
        {
            City* city = cityIt->second;
            int resourceid = co.parameters.resourceid;
            int price = prices.count(resourceid) ? prices[resourceid] : 1;
            bool ismfggood = resourceid >= rum;
            std::unordered_map<int,int>& stockpile = city->resources;

            // Same "top up a non-full stack, else take a new slot" rule as LoadCargoOrder:
            // a Transport with 2+ free slots can buy several separate 100-stacks of the same
            // resource (e.g. 230 elephants in the city -> 100 into slot 1, then 100 more into
            // slot 2), instead of being capped at 100 total the moment any stack exists.
            Resource* existing = findToppableCargo(transport, resourceid);
            int roomAboard;
            if (existing != nullptr)
                roomAboard = 100 - existing->amount;
            else
                roomAboard = (transport->manifest() < transport->capacity()) ? 100 : 0;

            int qty = std::min(std::min(100, stockpile[resourceid]), roomAboard);
            if (price > 0) qty = std::min(qty, treasury->resources[COINS] / price);

            if (qty > 0)
            {
                bool loaded = true;
                if (existing != nullptr)
                {
                    existing->amount += qty;
                }
                else
                {
                    Resource* cargoitem = ismfggood
                        ? (Resource*) new MfgGood(resourceid, tiles[resourceid].c_str(), "MfgGood")
                        : (Resource*) new Commodity(resourceid, tiles[resourceid].c_str(), "Commodity");
                    cargoitem->amount = qty;
                    loaded = transport->board(dynamic_cast<Shippable*>(cargoitem));
                    if (!loaded) { delete cargoitem; message(year, co.parameters.factionid, "No room aboard to buy more cargo."); }
                }

                if (loaded)
                {
                    int cost = qty * price;
                    stockpile[resourceid]          -= qty;
                    treasury->resources[COINS] -= cost;
                    city->resources[COINS]     += cost;
                }
            }
        }
    } else if (co.command == Command::SellResourceOrder)
    {
        // Commerce screen "sell": like UnloadCargoOrder, plus the coin transfer. Sells the
        // whole boarded stack, capped by what the city can pay at prices[resourceid]; the
        // proceeds go to the seller faction's capital-city treasury (see BuyResourceOrder).
        Transport* transport = dynamic_cast<Transport*>(units[co.parameters.spawnid]);
        auto cityIt = cities.find(co.parameters.cityid);
        City* treasury = factionTreasury(co.parameters.factionid);
        if (transport != nullptr && cityIt != cities.end() && treasury != nullptr)
        {
            City* city = cityIt->second;
            int resourceid = co.parameters.resourceid;
            int price = prices.count(resourceid) ? prices[resourceid] : 1;

            if (Resource* r = dynamic_cast<Resource*>(transport->findCargo(resourceid)))
            {
                int qty = r->amount;
                if (price > 0) qty = std::min(qty, city->resources[COINS] / price);

                if (qty > 0)
                {
                    int proceeds = qty * price;
                    bool ismfggood = resourceid >= rum;
                    std::unordered_map<int,int>& stockpile = city->resources;

                    stockpile[resourceid]          += qty;
                    city->resources[COINS]     -= proceeds;
                    treasury->resources[COINS] += proceeds;

                    r->amount -= qty;
                    if (r->amount <= 0)
                    {
                        transport->removeCargo(resourceid);
                        delete r;
                    }
                }
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

// Process the work of the active unit if it is working....
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

void capResources(City* c)
{
    int cap = 300; // @FIXME: Put this as a constant (same for the other values)

    // Capping the amount of commodities based on storage expansion technologies
    if (dee.verifyDep(cityContext(c->id), STORAGE_EXPANSION_1))
        cap = 600;
    if (dee.verifyDep(cityContext(c->id), STORAGE_EXPANSION_2))
        cap = 900;

    for(int r_id : ALL_COMMODITIES_AND_MFGGOODS)
    {
        if (c->resources[r_id] > cap)
            c->resources[r_id] = cap;
    }
}
