/* ============================================================================
**
** Main Program - Bunmei - 18/18/2023
**
** Copyright (C) 2014  faturita - Rodrigo Ramele
**
** For personal, educationnal, and research purpose only, this software is
** provided under the Gnu GPL (V.3) license. To use this software in
** commercial application, please contact the author.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License V.3 for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**
** ========================================================================= */

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdarg.h>
#include <math.h>

#include <cassert>
#ifdef __linux
#include <GL/glut.h>
#include <algorithm>
#elif __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#endif

#include <vector>

#include <iostream>
#include <unordered_map>
#include <algorithm>

#include "imageloader.h"
#include "profiling.h"
#include "commandline.h"
#include "font/DrawFonts.h"
#include "font/FontsBitmap.h"
#include "math/yamathutil.h"
#include "camera.h"
#include "openglutils.h"
#include "lodepng.h"
#include "tiles.h"
#include "usercontrols.h"
#include "coordinator.h"
#include "map.h"
#include "hud.h"
#include "ai.h"

#include "buildable.h"

#include "resources.h"
#include "Faction.h"
#include "diplomacy.h"
#include "gamekernel.h"
#include "engine.h"
#include "messages.h"

#include "sounds/sounds.h"

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
#include "City.h"

extern Controller controller;

std::unordered_map<int,std::queue<std::string>> citynames;

std::unordered_map<int, Unit*> units;
std::unordered_map<int, City*> cities;
std::vector<Faction*> factions;
DiplomacyTable diplomacy;
std::vector<Resource*> resources;
std::vector<Message> messages;

Coordinator coordinator;

extern DiplomacyTable diplomacy;
extern ImprovementEffort improvementeffort;


int REAL_SCREEN_WIDTH = 1728;
int REAL_SCREEN_HEIGHT = 1117;

extern Map map;

int year;

bool mute;

int mapsize;

bool preloadmap;

bool loadgame;
char filegame[256];

bool autoEndOfTurn;
bool switchVisibleFaction;


void disclaimer()
{
    char version[]="1.0.0";
    printf("Bunmei version %s\n", version);
}

void setupWorldModelling()
{
    initMap();

    if (loadgame)
        loadWorldModelling();
    else
        initWorldModelling();
}

void initRendering()
{

}


void drawScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //glMatrixMode(GL_PROJECTION);
        //glLoadIdentity();
        //gluPerspective(45.0, (float)1440 / (float)900, 1.0, Camera.pos[2]+ horizon /**+ yyy**/);


    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();


    //Vec3f up,pos,forward;
    //Camera.lookAtFrom(up, pos, forward);

    // Sets the camera and that changes the floor position.
    //Camera.setPos(pos);

    switch (controller.view)
    {
    case 1:case 2:
        drawMap();
        drawHUD();
        break;
    case 5:
        drawIntro();
        break;
    
    default:
        break;
    }

    glDisable(GL_TEXTURE_2D);

    glutSwapBuffers();
}


void checkUnitMeetings(Unit* u)
{
    int targetFactionId = -1;
    int activeFactionId = u->faction;

    targetFactionId = findNearbyEnemyFactionId(u->id, 5);

    if (targetFactionId != -1 && !factions[targetFactionId]->autoPlayer)
    {
        if (diplomacy[u->faction][targetFactionId].status == DiplomaticStatus::NO_CONTACT)
        {
            controller.query.active = true;
            char msg[128];
            snprintf(msg, sizeof(msg), "An emissary from %s offer you peace, would you accept it?", factions[u->faction]->name);
            factions[u->faction]->song();
            controller.query.message = msg;
            controller.query.options = {"Yes.", "No."};
            controller.query.selected = [activeFactionId,targetFactionId](int i)
            {
                if (i == 0)
                {
                    diplomacy[activeFactionId][targetFactionId].makePeace();
                    char msg[128];
                    snprintf(msg, sizeof(msg), "%s have declared peace with %s.", factions[activeFactionId]->name, factions[targetFactionId]->name);
                    message(year, activeFactionId, msg);
                    message(year, targetFactionId, msg);
                    peace();
                } else {
                    diplomacy[activeFactionId][targetFactionId].makeWar();
                    char msg[128];
                    snprintf(msg, sizeof(msg), "%s are at WAR with %s.", factions[activeFactionId]->name, factions[targetFactionId]->name);
                    message(year, activeFactionId, msg);
                    message(year, targetFactionId, msg);
                    war();
                }
                printf("Selected option %d\n", i);
            };
        }
    }
}

inline void endOfYear()
{
    year++;
    for (auto& [k, u] : units)
    {
        // Units in movement debt (negative moves) recover one year of moves at a time
        // instead of getting the full refresh: crossing a tile that costs more than the
        // unit's moves-per-turn takes several turns.
        if (u->availablemoves < 0)
            u->availablemoves += u->getUnitMoves();
        else
            u->availablemoves = u->getUnitMoves();

        if (u->hasPendingMove() && u->availablemoves >= 0)
            completePendingMove(u);

        checkUnitMeetings(u);

    }

    std::vector<int> todelete;
    for (auto& [k, c] : cities) 
    {
        // Pick two food items per one population and gather the rest.
        // If granary is present the amount of food that is required to increase the population is half.

        printf("City %s\t\t\thas %02d pop and %03d food\n",c->name,c->pop,c->resources[0]);
        // Go through all the map locations and gather all the resources.
        for(auto &r:resources)
        {
            c->resources[r->id] += c->getProductionRate(r->id);
        }

        // Reduce the number of resources according to what is required now.
        for(auto &r:resources)
        {
            c->resources[r->id] -= c->getConsumptionRate(r->id);
        }

        // Convert trade accordingly.  Trade is not accummulated

        c->resources[COINS] += (int)((float)c->resources[TRADE] * factions[c->faction]->rates[0]);
        c->resources[SCIENCE] += (int)((float)c->resources[TRADE] * factions[c->faction]->rates[1]);
        //c->resources[LUXURY] += (int)((float)c->resources[TRADE] * factions[c->faction]->rates[0])
        c->resources[CULTURE] += (int)((float)c->resources[TRADE] * factions[c->faction]->rates[2]);

        c->resources[TRADE]=0;
        

        // Peek the production queue.
        if (c->productionQueue.size()>0)
        {
            BuildableFactory *bf = c->productionQueue.front();
            if (c->resources[1]>=bf->cost(1))
            {
                c->resources[1] -= bf->cost(1);          // @FIXME This can be extended to more resources.

                // Access the production queue from the city, build the latest thing in the queue and move forward with the next one
                c->productionQueue.pop();
                Buildable *b = bf->create();

                if (b->getType() == BuildableType::UNIT)
                {
                    Unit *unit = (Unit*)b;
                    unit->longitude = c->longitude;
                    unit->latitude = c->latitude;
                    unit->id = getNextUnitId();
                    unit->faction = c->faction;
                    unit->availablemoves = unit->getUnitMoves();

                    units[unit->id] = unit;
                }
                else
                {
                    Building *building = (Building*)b;
                    building->faction = c->faction;
                    c->buildings.push_back(building);

                    message(year, c->faction, "City %s has built %s.",c->name,building->name);
                    built();                
                }

            }
        }
        
        // Balance city population according to available resources.
        if (c->resources[FOOD]>100*c->pop) 
        {
            c->resources[FOOD] = 0;
            c->pop++;

            c->assignWorkingTile();
        } else 
        if (c->resources[FOOD]<0)
        {
            c->resources[FOOD] = 0;

            if (c->pop>1)
            {
                c->pop--;
                c->deAssigntWorkingTile();
            } else if (c->pop == 1)
            {
                // The city is abandoned.
                todelete.push_back(c->id);

                // @FIXME: When a city is captured with pop 1 it should be burned.
            }

        }

    }

    for(auto& cid:todelete)
    {
        City* c = cities[cid];
        // The city is abandoned.
        message(year, c->faction, "%s has been abandoned.",c->name);

        c->pop = 0;
        c->deAssigntWorkingTile();
        map.set(c->latitude, c->longitude).releaseCityOwnership();  // The removing of the 0,0 tile.
        cities.erase(c->id);
        delete c;

        // @FIXME: Check the consistency of the map regarding that no deleted city should be still marked there
    }

    
    for(auto& f:factions)
    {
        f->ready();
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



void adjustMovements()
{
    if ( (coordinator.a_u_id != CONTROLLING_NONE) && units.find(coordinator.a_u_id) != units.end() && (controller.registers.pitch!=0 || controller.registers.roll !=0) )
    {
        // Receives real latitude and longitude (contained in the unit)
        int lon = units[coordinator.a_u_id]->longitude;
        int lat = units[coordinator.a_u_id]->latitude;

        // Affect the coordinates according to the desired movement.
        coordinate s = map.displacement(lat,lon,controller.registers.pitch,controller.registers.roll);
        lat = s.lat;
        lon = s.lon;
        coordinate c = map.to_real_without_offset(s);



        controller.registers.pitch= controller.registers.roll = 0;     

        printf("Lat %d Lon %d  -> (%d,%d) -> (%d,%d) Land %d  Bioma  %x  \n",units[coordinator.a_u_id]->latitude,units[coordinator.a_u_id]->longitude, lat,lon,c.lat, c.lon, map.set(c.lat,c.lon).code, map.set(c.lat,c.lon).bioma); 

        // Now move the unit if it is possible.
        moveUnit(units[coordinator.a_u_id],c.lat,c.lon);  

        switchUnitIfNoMovesLeft();
    }  

    if ( (controller.registers.yaw !=0) )
    {
        factions[coordinator.a_f_id]->mapoffset += controller.registers.yaw;
        controller.registers.yaw = 0;
    }   
    
    if ( (controller.registers.precesion !=0) )
    {
        factions[coordinator.a_f_id]->vmapoffset += controller.registers.precesion;
        controller.registers.precesion = 0;
    }
}


void processGoTo()
{
    // GoTo Function
    if (units.find(coordinator.a_u_id)!=units.end() && units[coordinator.a_u_id]->isAuto())
    {
        // First build the tree map of the available land.
        // Calculate the path to the target.

        bool ok = false;
        
        coordinate c = goTo(units[coordinator.a_u_id],ok);
        
        if (ok)
        {
            // Find which direction (i,j) leads from current position to next position c
            // by checking all 8 neighbors using map.adjust()
            bool found = false;
            int current_lat = units[coordinator.a_u_id]->latitude;
            int current_lon = units[coordinator.a_u_id]->longitude;
            
            for(int i=-1; i<=1 && !found; i++)
            {
                for(int j=-1; j<=1 && !found; j++)
                {
                    if (i==0 && j==0)
                        continue;
                    
                    coordinate neighbor = map.adjust(current_lat, current_lon, i, j);
                    
                    if (neighbor.lat == c.lat && neighbor.lon == c.lon)
                    {
                        controller.registers.pitch = i;
                        controller.registers.roll = j;
                        found = true;
                    }
                }
            }
            
            if (!found)
            {
                // The pathfinding returned a coordinate that is not a valid neighbor!
                // This shouldn't happen, but if it does, cancel the goto to avoid getting stuck
                printf("ERROR: Pathfinding returned non-neighbor coordinate! Current: (%d,%d), Target step: (%d,%d)\n", 
                       current_lat, current_lon, c.lat, c.lon);
                units[coordinator.a_u_id]->resetGoTo();
            }
            else if (units[coordinator.a_u_id]->getMovementType()==OCEANTYPE && map.peek(c.lat,c.lon).code==LAND)
            {
                // A naval unit's final step is onto its LAND target (disembark on a coast,
                // dock in a city).  A coast landing never moves the SHIP onto the target,
                // so arrived() would never clear the GoTo and it would re-trigger every
                // tick (stall): the landing step is one-shot.
                units[coordinator.a_u_id]->resetGoTo();
            }

        }
        else
        {
            // Cancel goto operation and make a sound.
            //if (!units[coordinator.a_u_id]->arrived()) blocked();
            units[coordinator.a_u_id]->resetGoTo();
        }

        units[coordinator.a_u_id]->arrived();

    }
}


// Update GAME Model
void update(int value)
{
    // Derive the control to the correct object
    if (controller.isInterrupted())
    {
        exit(0);
    }

    cleanUnits();

    reSetCities();

    processGoTo();

    processWork();

    // Autoplayer
    if (factions[coordinator.a_f_id]->autoPlayer)
    {
        autoPlayerMoveUnits();
    }

    processCommandOrders();

    adjustMovements();

    // @NOTE: Remove me if you want to wait until the user press the space bar to move ahead the end of turn.
    if (autoEndOfTurn && noMoreMovementsLeft(coordinator.a_f_id))
    {
        coordinator.endofturn = true;
    }

    if (coordinator.endofturn)
    {
        coordinator.endofturn=false;
        factions[coordinator.a_f_id]->done();

        if (coordinator.a_f_id<factions.size()-1) 
        {
            coordinator.a_f_id++;

            if (switchVisibleFaction)
                coordinator.v_f_id = coordinator.a_f_id;

            controller.reset();
            setUpFaction();  

            // Autoplayer
            if (factions[coordinator.a_f_id]->autoPlayer)
            {
                autoPlayerCities();
            }
            
            // @NOTE: Forcing the map centering only for the user player, not for the AI players.
            if (!(factions[coordinator.a_f_id]->autoPlayer))
                if (units.find(coordinator.a_u_id)!=units.end())
                {
                    map.setCenter(factions[coordinator.a_f_id]->vmapoffset,factions[coordinator.a_f_id]->mapoffset);
                    coordinate c(units[coordinator.a_u_id]->latitude,units[coordinator.a_u_id]->longitude);
                    c = map.to_screen(c.lat,c.lon);
                    centermapinmap(c.lat, c.lon);
                    resetzoom();
                }  
            }

    }

    if (endOfTurnForAllFactions())
    {
        // Everybody played their turn, end of year, and start it over.....
        endOfYear();
        coordinator.a_f_id = 0;     // Restart the turn from the first faction.
        
        controller.reset();
        setUpFaction();

        // Autoplayer
        if (factions[coordinator.a_f_id]->autoPlayer)
        {
            autoPlayerCities();
        }

        // @NOTE: Forcing the map centering only for the user player, not for the AI players.
        if (!(factions[coordinator.a_f_id]->autoPlayer))
            if (units.find(coordinator.a_u_id)!=units.end())
            {
                map.setCenter(factions[coordinator.a_f_id]->vmapoffset,factions[coordinator.a_f_id]->mapoffset);
                coordinate c(units[coordinator.a_u_id]->latitude,units[coordinator.a_u_id]->longitude);
                c = map.to_screen(c.lat,c.lon);
                centermapinmap(c.lat, c.lon);
                resetzoom();
            }  
    }

    glutPostRedisplay();
    // @NOTE: update time should be adapted to real FPS (lower is faster).
    glutTimerFunc(20, worldStep, 0);    
}



int main(int argc, char** argv) {
    glutInit(&argc, argv);

#ifdef DEBUG
    CLog::SetLevel(CLog::All);
#else
    CLog::SetLevel(CLog::None);
#endif

    if (isPresentCommandLineParameter(argc,argv,"-seed"))
    {
        int seed = getDefaultedIntCommandLineParameter(argc,argv,"-seed",0);
        srand( seed );
        srand48(seed);
        setRandomSeed(seed);        // getRandomInteger uses its own <random> generator.
    }
    else
    {
        srand (time(NULL));
        srand48(time(NULL));
    }

    // Switch up OpenGL version (at the time of writing compatible with 2.1)
    if (true)
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
#ifdef __linux

#elif __APPLE__
    else
        glutInitDisplayMode (GLUT_3_2_CORE_PROFILE | GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
#endif


    disclaimer();
    glutCreateWindow("Bunmei");

    if (isPresentCommandLineParameter(argc,argv,"-d"))
        glutInitWindowSize(1200, 800);
    else
        glutFullScreen();

    if (isPresentCommandLineParameter(argc,argv,"-mute"))
        mute = true;
    else
        mute = false;


    // OpenGL Configuration information
    /* get version info */
    const GLubyte* renderer;
    const GLubyte* version;

    renderer = glGetString (GL_RENDERER);
    version = glGetString (GL_VERSION);
    printf ("Renderer: %s\n", renderer);
    printf ("OpenGL version supported: %s\n", version);

    REAL_SCREEN_HEIGHT = glutGet(GLUT_SCREEN_HEIGHT);
    REAL_SCREEN_WIDTH = glutGet(GLUT_SCREEN_WIDTH);

    printf("Width:%d\n", glutGet(GLUT_SCREEN_WIDTH) );
    printf("Height:%d\n", glutGet(GLUT_SCREEN_HEIGHT) );

    mapsize = getDefaultedIntCommandLineParameter(argc,argv,"-mapsize",DEFAULT_MAPSIZE);

    preloadmap = false;
    if (isPresentCommandLineParameter(argc,argv,"-loadmap"))
    {
        preloadmap = true;
    }

    loadgame = false;
    if (isPresentCommandLineParameter(argc,argv,"-loadgame"))
    {
        loadgame = true;
        strcpy(filegame, getCommandLineParameter(argc,argv,"-loadgame"));
        struct stat buffer;
        if (stat(filegame, &buffer) != 0) {
            std::cerr << "Error: The file " << filegame << " does not exist." << std::endl;
            exit(1);
        }
    }

    setupWorldModelling();
    initRendering();
    initSound();

    preloadFonts();
    if (!isPresentCommandLineParameter(argc,argv,"-nointro") && !isPresentCommandLineParameter(argc,argv,"-test"))
        {intro();controller.view = 5;}


    // OpenGL callback functions.
    glutDisplayFunc(drawScene);
    glutKeyboardFunc(handleKeypress);
    //glutSpecialFunc(handleSpecKeypress);
    //glutIdleFunc(&update_fade_factor);

    // Resize callback function.
    //glutReshapeFunc(handleResize);

    //adding here the mouse processing callbacks
    glutMouseFunc(processMouse);
    glutMotionFunc(processMouseActiveMotion);
    glutPassiveMotionFunc(processMousePassiveMotion);
    glutEntryFunc(processMouseEntry);

    // this is the first time to call to update.
    glutTimerFunc(25, worldStep, 0);

    // main loop, hang here.
    glutMainLoop();


    return 1;
}