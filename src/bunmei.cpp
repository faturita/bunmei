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
#include "dee.h"

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
std::vector<Message> messages;

Coordinator coordinator;
DependencyEvaluationEngine dee;

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
bool nofog;
int  selectedFaction;

// -civs N: how many of the defined civilizations to load, clamped to [2,7] in
// initFactions() (gamekernel.cpp). -1 (unset) means "no cap, load every defined civ" --
// same sentinel convention as selectedFaction above.
int  numCivs;


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

    if (nofog)
        for(int lat=map.minlat;lat<map.maxlat;lat++)
            for (int lon=map.minlon;lon<map.maxlon;lon++)
            {
                for (int f_id=0;f_id<(int)factions.size();f_id++)
                    map.set(lat,lon).setVisible(f_id);
            }   
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

        printf("City %s\t\t\thas %02d pop and %03d food\n",c->name,c->pop,c->coreresources[FOOD]);
        // Go through all the map locations and gather all the resources.
        for(int r_id : ALL_CORE_RESOURCES)
        {
            c->coreresources[r_id] += c->getProductionRate(r_id);
        }

        // Commodities: gathered from every special resource within range regardless of
        // whether the tile is worked (see City::getCommodityProductionRate).
        for(int commodity_id : ALL_COMMODITIES)
        {
            c->commodities[commodity_id] += c->getCommodityProductionRate(commodity_id);
        }

        // Reduce the number of resources according to what is required now.
        for(int r_id : ALL_CORE_RESOURCES)
        {
            c->coreresources[r_id] -= c->getConsumptionRate(r_id);
        }

        // Convert trade accordingly.  Trade is not accummulated

        c->coreresources[COINS] += (int)((float)c->coreresources[TRADE] * factions[c->faction]->rates[0]);
        c->coreresources[SCIENCE] += (int)((float)c->coreresources[TRADE] * factions[c->faction]->rates[1]);
        //c->coreresources[LUXURY] += (int)((float)c->coreresources[TRADE] * factions[c->faction]->rates[0])
        c->coreresources[CULTURE] += (int)((float)c->coreresources[TRADE] * factions[c->faction]->rates[2]);

        c->coreresources[TRADE]=0;
        

        // Peek the production queue.
        if (c->productionQueue.size()>0)
        {
            BuildableFactory *bf = c->productionQueue.front();
            if (c->coreresources[SHIELDS]>=bf->cost(SHIELDS))
            {
                c->coreresources[SHIELDS] -= bf->cost(SHIELDS);          // @FIXME This can be extended to more resources.

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

                    if (dee.verifyDep(cityContext(c->id), VETERAN_CODE))  // Set the code name as something that depends on the feature
                    {
                        unit->veteran();
                    }

                    units[unit->id] = unit;
                }
                else
                {
                    Building *building = (Building*)b;
                    building->faction = c->faction;
                    c->buildings.push_back(building);

                    dee.regDep(cityContext(c->id), building->getPerkCodes());

                    message(year, c->faction, "City %s has built %s.",c->name,building->name);
                    built();                
                }

            }
        } else { // Reset resources if there is nothing to build.
            c->coreresources[SHIELDS] = 0;
            message(year, c->faction, "City %s has nothing to build.",c->name);
        }
        
        // Balance city population according to available resources.
        float popFactor = 0.0f;
        if (dee.verifyDep(cityContext(c->id), HALF_POPULATION_CODE))
        {
            popFactor = 0.5f;
        }
        if (c->coreresources[FOOD]>= getPopulationThresshold(c->pop))
        {
            c->pop++;

            c->coreresources[FOOD] = (int)(popFactor * (float)getPopulationThresshold(c->pop));  // Keep half of the food required for the NEXT growth (the new pop's thresshold, matching the Food Storage UI's line -- which is always drawn against the CURRENT pop) if the granary is present, otherwise it is a full loss.

            c->assignWorkingTile();
        } else
        if (c->coreresources[FOOD]<0)  // Out of food, reduce population accordingly.
        {
            c->coreresources[FOOD] = 0;
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

        CommandOrder co;
        co.command = Command::MoveUnitTo;
        co.parameters.spawnid = units[coordinator.a_u_id]->id;
        co.parameters.factionid = units[coordinator.a_u_id]->faction;
        co.parameters.latitude = lat;
        co.parameters.longitude = lon;
        coordinator.push(co);

        // Reset the controller registers to avoid moving again in the same direction.
        controller.registers.pitch= controller.registers.roll = 0;    
        
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

void switchFaction()
{
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

    adjustMovements();

    processCommandOrders();

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

            switchFaction();

        }

    }

    if (endOfTurnForAllFactions())
    {
        // Everybody played their turn, end of year, and start it over.....
        endOfYear();
        coordinator.a_f_id = 0;     // Restart the turn from the first faction.
        
        switchFaction();
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

    nofog = isPresentCommandLineParameter(argc,argv,"-nofog");

    selectedFaction = getDefaultedIntCommandLineParameter(argc,argv,"-faction",-1);

    numCivs = getDefaultedIntCommandLineParameter(argc,argv,"-civs",-1);

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