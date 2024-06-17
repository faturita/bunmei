#include "openglutils.h"
#include "font/DrawFonts.h"
#include "hud.h"


void drawHUD()
{
    // This will make things dark.

    //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1200, 800, 0, -1, 1);            // @NOTE: This is the size of the HUD screen, it goes from x=[0,1200] , y=[-400,400]
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glPushAttrib(GL_CURRENT_BIT);
    glColor4f(1.0f, 1.0f, 1.0f, 1);
    glDisable(GL_DEPTH_TEST);
    glRotatef(180.0f,0,0,1);
    glRotatef(180.0f,0,1,0);

    int aimc=0,crossc=0;

    char str[256];

    sprintf (str, "Bunmei - Vikings");
    // width, height, 0 0 upper left
    drawString(0,-30,1,str,0.2f);


    sprintf (str, "Population: %d", 3432);
    drawString(0,-60,1,str,0.2f);



    glPopAttrib();
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}
