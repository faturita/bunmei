#ifndef ENGINE_H
#define ENGINE_H

#include "Faction.h"
#include "City.h"
#include "units/Unit.h"
#include "units/Ship.h"

// Whether a unit of faction f_id may step onto cell, and whether doing so seizes it.  Free
// land and the mover's own land are always ENTER_AND_CLAIM (claiming your own land again just
// keeps the mapcell owners-stacking counter correct, see mapcell::setOwnedBy).  A foreign-owned
// cell depends on the diplomacy status between f_id and the owner (README.md DefCon table):
// landSeizure lets the mover in AND flips ownership, openBorders (without landSeizure) lets
// the mover in but leaves the tile with its original owner, and neither blocks the move.
enum class LandEntry { BLOCKED, ENTER, ENTER_AND_CLAIM };

int getNextCityId();
int getNextUnitId();
int nextUnitId(int faction);
int nextMovableUnitId(int f_id);

City* findCityAt(int lat, int lon);

// The city a faction's trade coins come from / go to (its capital, else its first city,
// else nullptr) -- the engine has no persistent per-faction coin pot.
City* factionTreasury(int faction_id);
Unit* getDefender(int lat, int lon, int &numberofdefenders, int f_id);

// Fills city->buildable with everything the city can currently build (based on the faction
// type -- this is all, for now). Does NOT clear the list first; callers that want a fresh
// list (e.g. Command::PopulateBuildableOrder) must clear() before calling.
void populateCityBuildables(City* city);

// Ask faction `factionId` what to research next. autoPlayer factions roll a random Frontier
// technology; a human one gets the controller.query selector. By default it only asks when the
// faction needs asking (no target yet, or the one it had left the Frontier); `force` asks even
// when the current target is still valid, which is what endOfYear() does after a discovery --
// the Frontier just widened, so the choice is worth revisiting. Also called when a faction
// founds its first city.
void chooseResearch(int factionId, bool force = false);

// Runs every Building in the city once for the year: a building that consumes commodities
// (Building::getConsumptionRate > 0 over ALL_COMMODITIES) and produces mfg goods
// (Building::getProductionRate > 0 over ALL_MFG_GOODS) -- e.g. a Factory turning iron into
// tools -- deducts its inputs from city->resources and adds its outputs to city->resources,
// but only if the city stocks enough of every input; a building short even one input
// produces and consumes nothing that year.  Called from bunmei.cpp's endOfYear().
void operateCityBuildings(City* c);

int findNearbyEnemyFactionId(int unitId, int radius);

// Makes u the active/selectable unit (coordinator.a_u_id) and wakes it out of whatever
// passive state it was in: a fortified/sentried unit packs up/wakes up, a working one
// (isWorking()) has its improvement interrupted (Unit::completed(), no finalize command --
// a later order starts the effort over from scratch).  Shared by the map-view unit click
// (usercontrols.cpp) and the city screen's stationed-unit icons (cityscreenui.cpp).
void activateUnit(Unit* u);

bool noMoreMovementsLeft(int fid);
void reSetCities();
void setUpFaction();
bool endOfTurnForAllFactions();

LandEntry evaluateLandEntry(int f_id, mapcell &cell);
void completePendingMove(Unit* unit);

void cleanUnits();

void processCommandOrders();
void processWork();

void switchUnitIfNoMovesLeft();
void moveUnit(Unit* unit, int lat, int lon);
Ship* findNavalUnit(int lat, int lon);
bool dockInCity(Unit* navalunit, int lat, int lon);
bool land(Unit* navalunit, int lat, int lon);
bool moveOntoNavalUnit(Unit* passenger, Ship* navalunit, int lat, int lon);
bool moveForward(Unit* unit, int lat, int lon);
bool captureCity(Unit* invader, int lat, int lon, bool &forceBreak);
bool attack(Unit* attacker, int lat, int lon, bool &forceBreak);

// A human Transport stepping onto a foreign city's tile at PEACE or better opens the
// commerce screen (controller.view = 4) instead of moving; returns true when it handled it.
bool engageTrade(Unit* unit, int lat, int lon);


#endif // ENGINE_H