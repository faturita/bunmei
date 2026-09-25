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

#include "version.h"
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
#include "infoui.h"
#include "marketui.h"
#include "automation.h"
#include "dee.h"
#include "technologies.h"

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
TechTree techtree;

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
int  numCivs;


void disclaimer()
{
    printf("Bunmei version %s\n", BUNMEI::version);
}

void setupWorldModelling()
{
    initMap();

    if (loadgame)
        loadWorldModelling();
    else
        initWorldModelling();

    // Whatever the starting units can see, revealed BEFORE the first frame: the world now
    // exists and its units are placed. This has to happen here and not in switchFaction() --
    // that only runs when the turn passes to a faction, so year -4000 rendered completely
    // black until the player pressed space, which is exactly what happened when the fog rule
    // moved out of the renderer (the renderer used to reveal on the first frame as a side
    // effect of drawing).
    updateFogOfWar();

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
    case 1:case 2:case 4:
        drawMap();
        drawHUD();
        break;
    case 3:
        drawInfoScreen();
        break;
    case 5:
        drawIntro();
        break;
    case 7:
        drawMarketScreen();
        break;

    default:
        break;
    }

    glDisable(GL_TEXTURE_2D);

    glutSwapBuffers();
}


// checkUnitMeetings() and endOfYear() moved to gamekernel.cpp: they are the TURN, not the
// client, and the simulator has to run exactly the same ones (its own copies had drifted --
// no salaries, no first contact, no TRADE/SCIENCE surplus perks, no building production).
// What stays here is only what needs a window: drawScene, adjustMovements' animation, and
// the GLUT callbacks below.

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

// @NOTE: TEST update function to see what are the actions that needs to be migrated to commands.
void update_test(int value)
{
    // Derive the control to the correct object
    if (controller.isInterrupted())
    {
        exit(0);
    }

    glutPostRedisplay();
    // @NOTE: update time should be adapted to real FPS (lower is faster).
    glutTimerFunc(20, worldStep, 0);  
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