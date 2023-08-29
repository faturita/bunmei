/* ============================================================================
**
** Main Program - Bunmei - 18/18/2023
**
** Copyright (C) 2014  Rodrigo Ramele
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

#ifdef __APPLE__
#include <GLUT/glut.h>
#elif __linux
#include <GL/glut.h>
#include <algorithm>
#endif

#include <vector>

#include <iostream>
#include <unordered_map>

#include "imageloader.h"
#include "profiling.h"
#include "commandline.h"
#include "font/DrawFonts.h"
#include "camera.h"
#include "openglutils.h"

#include "lodepng.h"

Camera Camera;

float horizon = 10000;




void disclaimer()
{
    char version[]="1.0.0";
    printf("Bunmei version %s\n", version);
}

void setupWorldModelling()
{

}

void initRendering()
{

}

void initSound()
{

}

// 1200 x 800
// 1920 x 1080
int width = 1200;
int height = 800;
int mapzoom=1;

float cx,cy;

std::unordered_map<std::string, GLuint> maptextures;


void placeMark(int x, int y, int size, const char* modelName)
{
    GLuint _texture;

    if (maptextures.find(std::string(modelName)) == maptextures.end())
    {
        // @FIXME: This means that the image is loaded every time this mark is rendered, which is wrong.
        //Image* image = loadBMP(modelName);
        //_texture = loadTexture(image);
        //delete image;

        unsigned char *img;

        unsigned w,h;

        lodepng_decode_file(&img, &w, &h, modelName, LCT_RGBA, 8);

        Image image((char *)img, w, h);

        glGenTextures(1, &_texture);
        glBindTexture(GL_TEXTURE_2D, _texture);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     w,h,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     img);
        //delete image;


        maptextures[std::string(modelName)]=_texture;

    } else {
        _texture = maptextures[std::string(modelName)];
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glColor4f(1.0f, 1.0f, 1.0f,1.0f);


    glBegin(GL_QUADS);
    //Front face
    glNormal3f(0.0, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-size / 2 + x, -size / 2 + y, 0);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(size / 2 + x, -size / 2 + y, 0);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(size / 2 + x, size / 2 + y, 0);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-size / 2 + x, size / 2 + y, 0);

    glEnd();

    // @NOTE: This is very important.
    glDisable(GL_TEXTURE_2D);
}

float X(float x)
{
    return (600-(x/1));
}

float Y(float y)
{
    return 0+(y/1);
}

void placeMarkLatLong(int lat, int longi, int iconsize, const char* modelName)
{
    placeMark(X(lat),Y(longi),iconsize,modelName);
}

void drawMap()
{
    // This will make things dark.

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    int xsize = width/mapzoom;
    int ysize = height/mapzoom;

    if (mapzoom==1)
    {
        cx = width/2;
        cy = height/2;
    }


    glOrtho(cx-xsize/2, cx+xsize/2, cy+ysize/2, cy-ysize/2, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glColor4f(1.0f, 1.0f, 1.0f, 1);
    glDisable(GL_DEPTH_TEST);
    glRotatef(180.0f,0,0,1);
    glRotatef(180.0f,0,1,0);



    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    char str[256];
    sprintf (str, "Kepler IV Sea");
    // width, height, 0 0 upper left
    drawString(0,-30,1,str,0.2f,1.0f,1.0f,1.0f);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); {
        glTranslatef(0, -400, 1);

        // Let's draw the typical grid of naval cartmaking.  It is 12x8. Each square is 100 kmf squared.
        // (+,0,0),(0,0,+) is upwards-right.  The screen is (+,0),(0,+) downward-right.
        for(int i=0;i<10;i++)
        {
            glLineWidth(2.5);
            glColor3f(0.0, 1.0, 0.0);
            glBegin(GL_LINES);
            glVertex3f(   1, -500+i*100, 0.0);
            glVertex3f(1200, -500+i*100, 0.0);
            glEnd();
        }

        for(int i=0;i<12;i++)
        {
            glLineWidth(2.5);
            glColor3f(0.0, 1.0, 0.0);
            glBegin(GL_LINES);
            glVertex3f(   1+i*100,  500, 0.0);
            glVertex3f(   1+i*100, -500, 0.0);
            glEnd();
        }

        glLineWidth(2.5);
        glColor3f(0.0, 1.0, 0.0);
        glBegin(GL_LINES);
        glVertex3f(   1200,  500, 0.0);
        glVertex3f(   1200, -500, 0.0);
        glEnd();

        glDepthMask(GL_FALSE);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


        for(int x=0;x<120;x++)
            for(int y=-50;y<50;y++)
            {
                //placeMark(x*10+5,y*10+5,10,"assets/assets/terrain/ocean.png");

            }


        //placeMark(30,90,10,"assets/assets/terrain/land.png");
        //placeMark(0,0,30,"assets/assets/units/trireme.png");
        //placeMark(600+5,0+5,10,"assets/assets/terrain/mountains_n.png");

        for(int x=-60;x<60;x++)
            for(int y=-50;y<50;y++)
            {
                placeMarkLatLong(x*10+5,y*10+5,10,"assets/assets/terrain/ocean.png");

            }

        placeMarkLatLong(0+5,0+5,10,"assets/assets/terrain/mountains_n.png");

        glDisable(GL_BLEND);

    } glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

}

void handleKeypress(unsigned char key, int x, int y) {
    if (key == 27) exit(0);
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

    glDisable(GL_TEXTURE_2D);

    glutSwapBuffers();
}


int main(int argc, char** argv) {
    glutInit(&argc, argv);

#ifdef DEBUG
    CLog::SetLevel(CLog::All);
#else
    CLog::SetLevel(CLog::None);
#endif

    if (isPresentCommandLineParameter(argc,argv,"-random"))
        srand (time(NULL));
    else
        srand (0);

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

    // OpenGL callback functions.
    glutDisplayFunc(drawScene);
    glutKeyboardFunc(handleKeypress);
    //glutSpecialFunc(handleSpecKeypress);
    //glutIdleFunc(&update_fade_factor);

    // Resize callback function.
    //glutReshapeFunc(handleResize);

    //adding here the mouse processing callbacks
    //glutMouseFunc(processMouse);
    //glutMotionFunc(processMouseActiveMotion);
    //glutPassiveMotionFunc(processMousePassiveMotion);
    //glutEntryFunc(processMouseEntry);

    // this is the first time to call to update.
    //glutTimerFunc(25, worldStep, 0);

    // main loop, hang here.
    glutMainLoop();


    return 1;
}
