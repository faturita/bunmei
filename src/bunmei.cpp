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

#include "imageloader.h"
#include "profiling.h"
#include "commandline.h"
#include "font/DrawFonts.h"
#include "font/FontsBitmap.h"
#include "math/yamathutil.h"
#include "camera.h"
#include "openglutils.h"
#include "lodepng.h"
#include "usercontrols.h"
#include "map.h"
#include "hud.h"

#include "buildable.h"

#include "resources.h"
#include "Faction.h"
#include "gamekernel.h"

#include "buildings/Building.h"
#include "buildings/Palace.h"
#include "buildings/Barracks.h"
#include "buildings/Granary.h"

#include "units/Unit.h"
#include "units/Settler.h"
#include "units/Warrior.h"
#include "units/Horseman.h"
#include "units/Worker.h"
#include "units/Trireme.h"
#include "City.h"

extern Controller controller;


std::unordered_map<int, Unit*> units;
std::unordered_map<int, City*> cities;
std::vector<Faction*> factions;
std::vector<Resource*> resources;

extern Map map;

int year;


void disclaimer()
{
    char version[]="1.0.0";
    printf("Bunmei version %s\n", version);
}

void setupWorldModelling()
{
    initMap();

    initResources();

    initFactions();

    controller.faction = factions[0]->id;
    controller.controllingid = nextUnitId(controller.faction);
    factions[0]->autoPlayer = false;
    
    year = -4000;

    centermapinmap(units[controller.controllingid]->latitude,units[controller.controllingid]->longitude);
    zoommapin();

}

void initRendering()
{

}

void initSound()
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

    drawMap();

    drawHUD();

    glDisable(GL_TEXTURE_2D);

    glutSwapBuffers();
}

inline void processCommandOrders()
{
    CommandOrder co = controller.pop();
    if (co.command == Command::BuildCityOrder)
    {
        City *city = new City();
        city->latitude = units[controller.controllingid]->latitude;
        city->longitude = units[controller.controllingid]->longitude;
        city->faction = units[controller.controllingid]->faction;
        city->id = getNextCityId();
        city->pop = 1;

        if (city->id==1)
        {
            city->setCapitalCity();
            // Buildings already built in the city
            city->buildings.push_back(new Palace());
        }

        // What the city can actually build.
        city->buildable.push_back(new BarracksFactory());
        city->buildable.push_back(new PalaceFactory());
        city->buildable.push_back(new SettlerFactory());
        city->buildable.push_back(new GranaryFactory());
        city->buildable.push_back(new WarriorFactory());
        city->buildable.push_back(new WorkerFactory());
        city->buildable.push_back(new HorsemanFactory());
        city->buildable.push_back(new TriremeFactory());

        // We add the Warrior as the first thing to build in the city.
        city->productionQueue.push(new WarriorFactory());

        
        cities[city->id] = city;

        // @FIXME: Disband the settler unit.

        Unit *settler = units[controller.controllingid];
        units.erase(controller.controllingid);
        delete settler;

        controller.controllingid = nextMovableUnitId(controller.faction, controller.controllingid);

    }
    else if (co.command == Command::DisbandUnitOrder)
    {
        Unit *unit = units[controller.controllingid];
        units.erase(controller.controllingid);
        delete unit;

        controller.controllingid = nextMovableUnitId(controller.faction,controller.controllingid);  //@FIXME: There could be the case that there are no more units.
    }    
}

inline void endOfYear()
{
    year++;
    for (auto& [k, u] : units) 
    {
        u->availablemoves = u->getUnitMoves();
    }
    controller.controllingid=nextUnitId(controller.faction);

    for (auto& [k, c] : cities) 
    {
        // Pick two food items per one population and gather the rest.
        // If granary is present the amount of food that is required to increase the population is half.

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
            }

        }

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

void worldStep(int value)
{
    // Derive the control to the correct object
    if (controller.isInterrupted())
    {
        exit(0);
    }

    if (factions[controller.faction]->autoPlayer)
    {
        if (getRandomInteger(0,1)==0)
        {
            controller.registers.roll = getRandomInteger(-1.0,1.0);
        }
        else
        {
            controller.registers.pitch = getRandomInteger(-1.0,1.0);
        }

        if (units.find(controller.controllingid)!=units.end())
        {
            if (units[controller.controllingid]->canBuildCity())
            {
                if (getRandomInteger(0,10)==0)
                {
                    CommandOrder co;
                    co.command = Command::BuildCityOrder;
                    controller.push(co);
                }
            }
        }

    }

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
            controller.reset();
            controller.faction++;
            controller.controllingid=nextUnitId(controller.faction);
            if (units.find(controller.controllingid)!=units.end())
            {
                coordinate c(units[controller.controllingid]->latitude,units[controller.controllingid]->longitude);
                c = map.to_fixed(c.lat,c.lon);
                centermapinmap(c.lat, c.lon);   
            }         
        }
        else
        {
            controller.reset();
            controller.faction=0;  // Reset for the new year.
            controller.controllingid=nextUnitId(controller.faction);
            if (units.find(controller.controllingid)!=units.end())
            {
                coordinate c(units[controller.controllingid]->latitude,units[controller.controllingid]->longitude);
                c = map.to_fixed(c.lat,c.lon);
                centermapinmap(c.lat, c.lon);
            }
        }
    }

    if (endOfTurnForAllFactions())
    {
        // Everybody played their turn, end of year, and start it over.....
        endOfYear();
    }

    processCommandOrders();


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
    else
        glutInitDisplayMode (GLUT_3_2_CORE_PROFILE | GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);

    disclaimer();
    glutCreateWindow("Bunmei");

    if (isPresentCommandLineParameter(argc,argv,"-d"))
        glutInitWindowSize(1200, 800);
    else
        glutFullScreen();


    // OpenGL Configuration information
    /* get version info */
    const GLubyte* renderer;
    const GLubyte* version;

    renderer = glGetString (GL_RENDERER);
    version = glGetString (GL_VERSION);
    printf ("Renderer: %s\n", renderer);
    printf ("OpenGL version supported: %s\n", version);

    setupWorldModelling();
    initRendering();
    initSound();

    preloadFonts();

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
