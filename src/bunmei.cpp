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

#include "resources.h"
#include "Faction.h"
#include "gamekernel.h"

#include "units/Unit.h"
#include "units/Settler.h"
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

    controller.faction = factions[1]->id;
    controller.controllingid = nextUnitId(controller.faction);
    
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




void worldStep(int value)
{
    // Derive the control to the correct object
    if (controller.isInterrupted())
    {
        exit(0);
    }

    if (controller.endofturn)
    {
        controller.endofturn=false;
        for (auto& [k, u] : units) 
        {
            if (u->faction == controller.faction)
            {
                u->availablemoves = u->getUnitMoves();
            }
        }
        year++;
        controller.controllingid=nextUnitId(controller.faction);

        for (auto& [k, c] : cities) 
        {
            // Go through all the map locations and gather all the resources.
            for(int lat=-3;lat<=3;lat++)
                for(int lon=-3;lon<=3;lon++)
                {
                    if (c->workingOn(lat,lon))
                    {
                        // This is the core game logic
                        for(auto &r:resources)
                        {
                            c->resources[r->id] += map(c->latitude+lat,c->longitude+lon).resource_production_rate[r->id];
                        }
                    }
                }

            if (c->resources[1]>50)
            {
                c->resources[1] = 0;

                // Production finished...
                Settler *settler = new Settler();
                settler->longitude = c->longitude;
                settler->latitude = c->latitude;
                settler->id = getNextUnitId();
                settler->faction = c->faction;
                settler->availablemoves = settler->getUnitMoves();


                units[settler->id] = settler;        
            }



            // Update summarized city values
            c->tick();
        }


    }

    CommandOrder co = controller.pop();
    if (co.command == Command::BuildCityOrder)
    {
        City *city = new City();
        city->latitude = units[controller.controllingid]->latitude;
        city->longitude = units[controller.controllingid]->longitude;
        city->faction = units[controller.controllingid]->faction;
        city->id = getNextCityId();
        city->pop = 1;
        
        cities[city->id] = city;

        // @FIXME: Disband the settler unit.

        Unit *settler = units[controller.controllingid];
        units.erase(controller.controllingid);
        delete settler;

        controller.controllingid = nextUnitId(controller.faction);

    }
    else if (co.command == Command::DisbandUnitOrder)
    {
        Unit *unit = units[controller.controllingid];
        units.erase(controller.controllingid);
        delete unit;

        controller.controllingid = nextUnitId(controller.faction);  //@FIXME: There could be the case that there are no more units.
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
