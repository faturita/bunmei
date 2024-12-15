#include "openglutils.h"
#include "profiling.h"
#include "usercontrols.h"
#include "map.h"

#include <assert.h>
#include <string>
#include <iostream>

Controller controller;


// Mouse offset for camera zoom in and out.
int _xoffset = 0;
int _yoffset = 0;


void handleKeypress(unsigned char key, int x, int y) {
    switch (key) {
        case 27: //Escape key
            controller.interrupt();
        case 'a':controller.registers.roll-=1.0f;break;
        case 'd':controller.registers.roll+=1.0f;break;
        case 'w':controller.registers.pitch+=1.0;break;
        case 's':controller.registers.pitch-=1.0;break;
        case 'q':controller.registers.pitch+=1.0;controller.registers.roll-=1.0f;break;
        case 'e':controller.registers.pitch+=1.0;controller.registers.roll+=1.0f;break;
        case 'z':controller.registers.pitch-=1.0;controller.registers.roll-=1.0f;break;
        case 'c':controller.registers.pitch-=1.0;controller.registers.roll+=1.0f;break;
        case 'f':controller.registers.yaw+=1.0;break;
        case 'g':controller.registers.yaw-=1.0;break;
        case ' ':controller.endofturn=true;break;
    default:break;
    }
}



void processMouseEntry(int state) {
    /**if (state == GLUT_LEFT)
        _angle = 0.0;
    else
        _angle = 1.0;**/

}


void processMouse(int button, int state, int x, int y) 
{
    int specialKey = glutGetModifiers();
    // if both a mouse button, and the ALT key, are pressed  then


    // This should not be here @FIXME
    //glutSetCursor(GLUT_CURSOR_NONE);


    if ((state == GLUT_DOWN) && button == GLUT_LEFT_BUTTON)
    {

    }


    if ((state == GLUT_DOWN)) 
    {
        _xoffset = _yoffset = 0;

        // @NOTE: On linux the right button on the touchpad sometimes do not work.
        if (GLUT_RIGHT_BUTTON == button || GLUT_MIDDLE_BUTTON == button)
        {
            //buttonState = 0;
            zoommapout();
        }
        // set the color to pure red for the left button
        if (button == GLUT_LEFT_BUTTON) 
        {
            //buttonState = 1;
            CLog::Write(CLog::Debug,"Mouse down %d,%d\n",x,y);
            centermap(x,y);

            if (specialKey == GLUT_ACTIVE_SHIFT)
            {
                zoommapin();
            } 
        }
    }
        // set the color to pure green for the middle button
    else if (button == GLUT_MIDDLE_BUTTON) 
    {

    }
    // set the color to pure blue for the right button
    else 
    {

    }

}



void processMouseActiveMotion(int x, int y) {

    if (_xoffset ==0 ) _xoffset = x;

    if (_yoffset == 0 ) _yoffset = y;

}




// Movement of the mouse alone.
void processMousePassiveMotion(int x, int y) {

//    if (buttonState==1)
//    {
//        controller.registers.pitch = ( (y-_yoffset) * 0.08);   // @FIXME these parameters should be dependant on what are you moving.
//        controller.registers.roll = ( (x - _xoffset ) * 0.05) ;
//    }



    //int specialKey = glutGetModifiers();
    // User must press the SHIFT key to change the
    // rotation in the X axis
    //if (specialKey != GLUT_ACTIVE_SHIFT) {

        // setting the angle to be relative to the mouse
        // position inside the window
    //if (_xoffset ==0 ) _xoffset = x;

    //if (_yoffset == 0 ) _yoffset = y;

    //if (buttonState == 1)
    {
    //    Camera.xAngle += ( (x-_xoffset) * 0.005);

    //    Camera.yAngle += ( (y - _yoffset ) * 0.005) ;
    }
    //processMouseActiveMotion(x,y);
}
