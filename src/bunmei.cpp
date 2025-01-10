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
#include "map.h"
#include "hud.h"
#include "ai.h"

#include "buildable.h"

#include "resources.h"
#include "Faction.h"
#include "gamekernel.h"
#include "engine.h"
#include "messages.h"

#include "sounds/sounds.h"

#include "buildings/Building.h"
#include "buildings/Palace.h"
#include "buildings/Barracks.h"
#include "buildings/Granary.h"
#include "buildings/Collosseum.h"

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
#include "City.h"

extern Controller controller;

std::unordered_map<int,std::queue<std::string>> citynames;

std::unordered_map<int, Unit*> units;
std::unordered_map<int, City*> cities;
std::vector<Faction*> factions;
std::vector<Resource*> resources;
std::vector<Message> messages;


int REAL_SCREEN_WIDTH = 1728;
int REAL_SCREEN_HEIGHT = 1117;

extern Map map;

int year;

bool mute;


void disclaimer()
{
    char version[]="1.0.0";
    printf("Bunmei version %s\n", version);
}

void setupWorldModelling()
{
    initMap();

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


inline void processCommandOrders()
{
    CommandOrder co = controller.pop();
    if (co.command == Command::BuildCityOrder)
    {
        // You cannot build a city in a land CLAIMED already by another city.
        if (!map.set(units[controller.controllingid]->latitude,units[controller.controllingid]->longitude).isUnassignedLand())
        {
            message(year, controller.faction, "City cannot be built here.  The land is already claimed by another city.");
            return;
        }


        City *city = new City(&map, units[controller.controllingid]->faction,getNextCityId(),units[controller.controllingid]->latitude,units[controller.controllingid]->longitude);
        city->setName(citynames[controller.faction].front().c_str());citynames[controller.faction].pop();

        // @NOTE: When the population is zero, the first city is the capital city.
        if (factions[controller.faction]->pop==0)
        {
            city->setCapitalCity();
            // Buildings already built in the city
            city->buildings.push_back(new Palace());

            // @FIXME: This is the momento to make the song of the faction.
            //russians();

        }

        // What the city can actually build.
        city->buildable.push_back(new BarracksFactory());
        city->buildable.push_back(new PalaceFactory());
        city->buildable.push_back(new SettlerFactory());
        city->buildable.push_back(new GranaryFactory());
        city->buildable.push_back(new CollosseumFactory());
        city->buildable.push_back(new WarriorFactory());
        city->buildable.push_back(new ArcherFactory());
        city->buildable.push_back(new SpearmanFactory());
        city->buildable.push_back(new SwordmanFactory());
        city->buildable.push_back(new AxemanFactory());
        city->buildable.push_back(new WorkerFactory());
        city->buildable.push_back(new HorsemanFactory());
        city->buildable.push_back(new TriremeFactory());
        city->buildable.push_back(new GalleyFactory());
        city->buildable.push_back(new HorsearcherFactory());


        // We add the Warrior as the first thing to build in the city.
        city->productionQueue.push(new WarriorFactory());

        
        cities[city->id] = city;

        // @FIXME: Disband the settler unit.
        Unit *settler = units[controller.controllingid];
        map.set(settler->latitude,settler->longitude).releaseOwner();
        units.erase(controller.controllingid);
        delete settler;

        controller.controllingid = nextMovableUnitId(controller.faction);

        message(year, controller.faction, "City %s %shas been founded.",city->name, city->isCapitalCity()?"(Capital) ":"");


    }
    else if (co.command == Command::DisbandUnitOrder)
    {
        Unit *unit = units[controller.controllingid];
        map.set(unit->latitude,unit->longitude).releaseOwner();
        units.erase(controller.controllingid);
        delete unit;

        controller.controllingid = nextMovableUnitId(controller.faction);  //@FIXME: There could be the case that there are no more units.
    }
    else if (co.command == Command::FortifyUnitOrder)
    {
        Unit *unit = units[controller.controllingid];
        unit->fortify();
        unit->availablemoves = 0;

        controller.controllingid = nextMovableUnitId(controller.faction);
    }
}

inline void endOfYear()
{
    year++;
    for (auto& [k, u] : units) 
    {
        u->availablemoves = u->getUnitMoves();
    }

    std::vector<int> todelete;
    for (auto& [k, c] : cities) 
    {
        // Pick two food items per one population and gather the rest.
        // If granary is present the amount of food that is required to increase the population is half.

        printf("City %s\t\thas %02d pop and %03d food\n",c->name,c->pop,c->resources[0]);
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
        if (c->resources[0]>100)
        {
            c->resources[0] = 0;
            c->pop++;

            c->assignWorkingTile();
        } else 
        if (c->resources[0]<0)
        {
            c->resources[0] = 0;

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

inline bool endOfTurnForAllFactions()
{
    for(auto& f:factions)
    {
        if (!f->isDone())
            return false;
    }
    return true;
}

bool war = true;


bool attack(Unit* attacker, int lat, int lon)
{
    std::vector<int> unitstodelete;
    bool confirmed = false;

    // War movement towards an enemy unit
    if (war && !map.set(lat,lon).isFreeLand() && !map.set(lat,lon).isOwnedBy(attacker->faction))
    {
        // Find the enemy unit located there
        Unit *defender = nullptr;
        int numberofdefenders = 0;
        for (auto& [k, u] : units) 
        {
            if (u->latitude == lat && u->longitude == lon && u->faction != controller.faction)
            {
                // @NOTE: How to pick which defender.  This should be rule-based.
                defender = u;
                numberofdefenders++;
            }
        }

        Unit *winner = nullptr;
        Unit *loser = nullptr;
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

        City* city = findCityAt(lat,lon);

        if (winner == attacker)
        {
            winner->availablemoves--;
            if (city == nullptr)
            {

                if (numberofdefenders==1)
                {
                    // Move the unit into the tile if there are no more units left AND if the tile is not a city.
                    coordinate c = map.to_real(lat,lon);

                    map.set(winner->latitude, winner->longitude).releaseOwner();

                    // Confirm the change if the movement is allowed.
                    attacker->update(lat,lon);

                    map.set(winner->latitude, winner->longitude).setOwnedBy(winner->faction);

                    winner->availablemoves=0;
                    
                }
            }
        }

        if (loser)
        {
            // Check what happens with All the other loosers.

            unitstodelete.push_back(loser->id);
            loser->availablemoves=0;

            if (loser == attacker)
            {
                controller.controllingid = nextMovableUnitId(controller.faction);
                printf("Lost\n");
            } 

        }
        printf("Attack condition\n");
        confirmed = true;

    }   

    for(auto& uid:unitstodelete)
    {
        Unit* u = units[uid];

        map.set(u->latitude, u->longitude).releaseOwner();

        units.erase(u->id);
        delete u;
    } 

    return confirmed;
}


bool captureCity(Unit* invader, int lat, int lon)
{
    // Move into an empty city.
    if (war && map.set(lat,lon).belongsToCity())
    {
        // Find the city located there
        City *city = findCityAt(lat,lon);

        if (city!=nullptr)
        {
            // Check if the city is not defended.
            if (city->faction != invader->faction && !city->isDefendedCity())
            {

                map.set(invader->latitude, invader->longitude).releaseOwner();

                invader->update(lat,lon);

                map.set(invader->latitude, invader->longitude).setOwnedBy(invader->faction);

                invader->availablemoves=0;   

                // Perhaps we should do some form of cleaning first, and a reassignment.
                city->reAssignWorkingTiles(invader->faction);
                city->faction = invader->faction;
                city->setDefense();              

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

    // March into a new tile (only allows movement in the tiles that I own @FIXME)
    if (map.set(lat,lon).isFreeLand() || (map.set(lat,lon).isOwnedBy(unit->faction)))
    {
        map.set(unit->latitude, unit->longitude).releaseOwner();

        // Normal, regular movement....
        unit->update(lat,lon);  

        // @FIXME: It should consider the terrain.
        unit->availablemoves--;

        map.set(unit->latitude, unit->longitude).setOwnedBy(unit->faction);

        printf("Move forward condition\n");
        return true;

    }

    return false;
 
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
        if(Trireme* trireme = dynamic_cast<Trireme*>(units[controller.controllingid]))
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

void moveUnit(Unit* unit, int lat, int lon)
{
    if (unit->availablemoves>0)
    {

        // Find a naval unit in the target tile.
        Trireme* navalunit = findNavalUnit(lat,lon);


        // @NOTE: moving into a ship
        if ((map.set(lat,lon).code==LAND && unit->getMovementType()==LANDTYPE) || (map.set(lat,lon).code==OCEAN && navalunit!=nullptr) ||
            (map.set(lat,lon).code==OCEAN && unit->getMovementType()==OCEANTYPE) || (map.set(lat,lon).code==LAND && dynamic_cast<Trireme*>(unit)->manifest()>0) )
        {

            if (!land(unit,lat,lon) && !moveOntoNavalUnit(unit, navalunit,lat,lon) && !captureCity(unit,lat,lon) && !attack(unit,lat,lon) && !moveForward(unit,lat,lon))
            {
                if (!war)
                {
                    factions[controller.faction]->blinkingrate = 10;
                    blocked();
                }   
            } 

        } else
        {
            factions[controller.faction]->blinkingrate = 10;
            blocked();  // @FIXME: differentiate between controlling unit and activeunit (active is what i am currently using indeed)
        }
    }
}

void switchUnitIfNoMovesLeft()
{
    if (controller.controllingid != CONTROLLING_NONE)
        if (units.find(controller.controllingid)!=units.end())
            if (units[controller.controllingid]->availablemoves==0)
            {
                int cid = nextMovableUnitId(controller.faction);
                if (cid != CONTROLLING_NONE)
                {
                    controller.controllingid = cid;
                }
                else
                {
                    controller.endofturn = true;
                }
            }
}

void adjustMovements()
{
    if ( (controller.controllingid != CONTROLLING_NONE) && (controller.registers.pitch!=0 || controller.registers.roll !=0) )
    {
        // Receives real latitude and longitude (contained in the unit)
        int lon = units[controller.controllingid]->longitude;
        int lat = units[controller.controllingid]->latitude;

        // Affect the coordinates according to the desired movement.
        coordinate s = map.spheroid_displacement(lat,lon,controller.registers.pitch,controller.registers.roll);
        lat = s.lat;
        lon = s.lon;
        coordinate c = map.to_real_without_offset(s);

        controller.registers.pitch= controller.registers.roll = 0;     

        printf("Lat %d Lon %d  -> (%d,%d) -> (%d,%d) Land %d  Bioma  %x  \n",units[controller.controllingid]->latitude,units[controller.controllingid]->longitude, lat,lon,c.lat, c.lon, map.set(c.lat,c.lon).code, map.set(c.lat,c.lon).bioma); 

        // Now move the unit if it is possible.
        moveUnit(units[controller.controllingid],c.lat,c.lon);  

        switchUnitIfNoMovesLeft();
    }  

    if ( (controller.registers.yaw !=0) )
    {
        factions[controller.faction]->mapoffset += controller.registers.yaw;
        controller.registers.yaw = 0;
    }      
}

void autoPlayer()
{
    if (units.find(controller.controllingid)!=units.end())
    {
        Unit *unit = units[controller.controllingid];
        if(Settler* s = dynamic_cast<Settler*>(units[controller.controllingid]))
        {
            if (!s->isAuto())
            {
                // Find the nearest city, and move AWAY from it as far as possible, 
                //    and if there is no city, build it here.
                City* nc = nullptr;
                int distance = 5;
                for(auto& [cid,c]:cities)
                {
                    int d = abs(c->latitude-s->latitude)+abs(c->longitude-s->longitude);  // Manhattan distance
                    if (d<distance)
                    {
                        distance = d;
                        nc = c;
                    }
                }

                if (nc!=nullptr)
                {
                    // Move away from the city.
                    bool ok;
                    coordinate c = reachableHorizon(s,5,s->faction,ok);
                    if (ok && !map.peek(c.lat,c.lon).isFreeLand())
                    {
                        s->goTo(c.lat,c.lon);
                    }
                    else
                    {
                        // Boludeo
                        controller.registers.roll = getRandomInteger(-1.0,1.0);
                        controller.registers.pitch = getRandomInteger(-1.0,1.0);

                        if (controller.registers.roll==0 && controller.registers.pitch==0)
                        {
                            s->availablemoves = 0;
                        }
                    }

                }
                else
                {
                    if (s->canBuildCity())
                    {
                        CommandOrder co;
                        co.command = Command::BuildCityOrder;
                        controller.push(co);
                    }
                }
            }

        }
        else
        {
            // If the unit is already in a city, fortify it.
            City* nc = nullptr;
            if(Warrior* w = dynamic_cast<Warrior*>(units[controller.controllingid]))
            for(auto& [cid,c]:cities)
            {
                if (c->faction == unit->faction && c->getCoordinate()==unit->getCoordinate())
                {
                    nc = c;
                    CommandOrder co;
                    co.command = Command::FortifyUnitOrder;
                    controller.push(co);
                }
            }
            
            if (nc == nullptr)
            {
                // If there is a defenseless enemy city nearby, capture it.
                City* cc = nullptr;
                for(auto& [cid,c]:cities)
                {
                    if (c->faction != unit->faction && !c->isDefendedCity())
                    {
                        cc = c;
                    }
                }

                if (cc == nullptr)
                {
                    // Boludeo
                    controller.registers.roll = getRandomInteger(-1.0,1.0);
                    controller.registers.pitch = getRandomInteger(-1.0,1.0);

                    if (controller.registers.roll==0 && controller.registers.pitch==0)
                    {
                        unit->availablemoves = 0;
                    }
                } else {
                    unit->goTo(cc->latitude,cc->longitude);
                }
            }
        }

    }

    // Control city production.
    for(auto& [k,c]:cities)
    {
        if (c->faction == controller.faction)
        {
            if (c->productionQueue.size()==0)
            {
                if (c->pop>1)
                {
                    int rand = getRandomInteger(0,2);

                    switch (rand) 
                    {
                        case 0:
                            c->productionQueue.push(new WarriorFactory());
                            break;
                        case 1:
                            c->productionQueue.push(new SettlerFactory());
                            break;
                        case 2:
                            c->productionQueue.push(new HorsemanFactory());
                            break;
                        default:
                            c->productionQueue.push(new ArcherFactory());
                    }
                }
            }
        }
    }
}

void setUpFaction()
{
    controller.reset();
    controller.controllingid=nextMovableUnitId(controller.faction);
    if (units.find(controller.controllingid)!=units.end())
    {
        map.setCenter(0,factions[controller.faction]->mapoffset);
        coordinate c(units[controller.controllingid]->latitude,units[controller.controllingid]->longitude);
        c = map.to_screen(c.lat,c.lon);
        centermapinmap(c.lat, c.lon);   
    }      
}

// GAME Model Update
void update(int value)
{
    // Derive the control to the correct object
    if (controller.isInterrupted())
    {
        exit(0);
    }

    for(auto& f:factions)
    {
        //printf("Faction %d - %s red %d\n",f->id,factions[f->id]->name,f->red);
        f->pop = 0;
        f->coins = 0;
    }

    // Update all the time if the city is or not defended...
    for(auto& [cid,c]:cities)
    {
        factions[c->faction]->pop += c->pop;
        c->noDefense();
        for(auto& [k, u] : units) 
        {
            if (u->latitude == c->latitude && u->longitude == c->longitude)
            {
                c->setDefense();
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


    // GoTo Function
    if (units.find(controller.controllingid)!=units.end() && units[controller.controllingid]->isAuto())
    {
        // First build the tree map of the available land.
        // Calculate the path to the target.

        bool ok = false;
        
        coordinate c = goTo(units[controller.controllingid],ok);
        
        if (ok)
        {
            controller.registers.roll = sgnz(c.lon-units[controller.controllingid]->longitude );
            controller.registers.pitch = sgnz(c.lat-units[controller.controllingid]->latitude );
        }
        else
        {
            // Cancel goto operation and make a sound.
            if (!units[controller.controllingid]->arrived()) blocked();
            units[controller.controllingid]->resetGoTo();
        }

        units[controller.controllingid]->arrived();

    }


    // Autoplayer
    if (factions[controller.faction]->autoPlayer)
    {

        autoPlayer();

    }

    adjustMovements();

    if (controller.endofturn)
    {
        controller.endofturn=false;
        factions[controller.faction]->done();

        // Reset all the remaining available moves for all the units that belong to the controller faction.
        for (auto& [k, u] : units) 
        {
            if (u->faction == controller.faction) u->availablemoves = 0;
        }

        if (controller.faction<factions.size()-1) 
        {
            controller.faction++;
            setUpFaction();    
        }

    }

    processCommandOrders();
    
    if (endOfTurnForAllFactions())
    {
        // Everybody played their turn, end of year, and start it over.....
        endOfYear();
        controller.faction = 0;     // Restart the turn from the first faction.
        setUpFaction();
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
