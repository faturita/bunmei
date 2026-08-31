#ifndef ENGINE_H
#define ENGINE_H

#include "Faction.h"
#include "City.h"
#include "units/Unit.h"
#include "units/Trireme.h"

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
Unit* getDefender(int lat, int lon, int &numberofdefenders, int f_id);

// Fills city->buildable with everything the city can currently build (based on the faction
// type -- this is all, for now). Does NOT clear the list first; callers that want a fresh
// list (e.g. Command::PopulateBuildableOrder) must clear() before calling.
void populateCityBuildables(City* city);

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
Trireme* findNavalUnit(int lat, int lon);
Trireme* findNavalUnit(int lat, int lon);
bool dockInCity(Unit* navalunit, int lat, int lon);
bool land(Unit* navalunit, int lat, int lon);
bool moveOntoNavalUnit(Unit* passenger, Trireme* navalunit, int lat, int lon);
bool moveForward(Unit* unit, int lat, int lon);
bool captureCity(Unit* invader, int lat, int lon, bool &forceBreak);
bool attack(Unit* attacker, int lat, int lon, bool &forceBreak);


#endif // ENGINE_H