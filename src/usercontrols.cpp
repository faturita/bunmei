#include <assert.h>
#include <string>
#include <iostream>

#include "openglutils.h"
#include "profiling.h"
#include "map.h"
#include "City.h"
#include "units/Unit.h"
#include "cityscreenui.h"
#include "engine.h"
#include "usercontrols.h"


Controller controller;
extern std::unordered_map<int, City*> cities;
extern std::unordered_map<int, Unit*> units;
extern Map map;

// Mouse offset for camera zoom in and out.
int _xoffset = 0;
int _yoffset = 0;

bool goToMode = false;


void handleKeypress(unsigned char key, int x, int y) {
    switch (key) {
        case 27: //Escape key
        {
            if (controller.view == 2)
            {
                controller.view = 1;
            } else {
                controller.interrupt();
            }
        }
        break;
        case 'a':controller.registers.roll-=1.0f;break;
        case 'd':controller.registers.roll+=1.0f;break;
        case 'w':controller.registers.pitch-=1.0;break;
        case 's':controller.registers.pitch+=1.0;break;
        case 'q':controller.registers.pitch-=1.0;controller.registers.roll-=1.0f;break;
        case 'e':controller.registers.pitch-=1.0;controller.registers.roll+=1.0f;break;
        case 'z':controller.registers.pitch+=1.0;controller.registers.roll-=1.0f;break;
        case 'c':controller.registers.pitch+=1.0;controller.registers.roll+=1.0f;break;
        case 'f':controller.registers.yaw+=1.0;break;
        case 'g':controller.registers.yaw-=1.0;break;
        case ' ':controller.endofturn=true;break;
        case 'G':
        {
             if (units.find(controller.controllingid) != units.end())
            {
                goToMode = true;
            }           
        }
        case 'C':
        {
            if (units.find(controller.controllingid) != units.end())
            {
                coordinate c(units[controller.controllingid]->latitude,units[controller.controllingid]->longitude);
                c = map.to_fixed(c.lat,c.lon);
                centermapinmap(c.lat, c.lon);
                resetzoom();
            }
        }
        break;
        case 9:
        {
            controller.controllingid = nextMovableUnitId(controller.faction, controller.controllingid);
            printf("Controlling %d\n",controller.controllingid);
        }
        break;
        case 'b':
        {
            if (units[controller.controllingid]->canBuildCity())
            {
                CommandOrder co;
                co.command = Command::BuildCityOrder;
                controller.push(co);
            }
        }
        break;
        case 'D':
        {
            CommandOrder co;
            co.command = Command::DisbandUnitOrder;
            controller.push(co);
        }
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

        switch (controller.view)
        {
            case 1:
            {
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
                    if (goToMode)
                    {
                        if (units.find(controller.controllingid) != units.end())
                        {
                            coordinate co = getCurrentCenter();
                            units[controller.controllingid]->goTo(co.lat,co.lon);
                        }
                        goToMode = false;
                    }
                    else
                    {
                        centermap(x,y);
                    }

                    if (specialKey == GLUT_ACTIVE_SHIFT)
                    {
                        zoommapin();
                    } else {
                        //controller.view = 2;
                        for (auto& [k,c] : cities) 
                        {
                            coordinate co = getCurrentCenter();
                            if (co.lat == c->latitude && co.lon == c->longitude)
                            {
                                printf("City %s %d %d,%d\n",c->name, c->id, c->latitude, c->longitude);
                                controller.view = 2;
                                controller.cityid = c->id;
                                break;
                            }
                        }
                    }
                }
                break;
            }
            case 2:
            {
                if (button == GLUT_LEFT_BUTTON) 
                {
                    CLog::Write(CLog::Debug,"Mouse down %d,%d\n",x,y);
                    printf("Clicked City %d\n",controller.cityid);
                    coordinate c = convertToMap(x,y,32);
                    printf("Location on the World Map (Lat,Lon)= (%d,%d)\n",c.lat,c.lon);
                    coordinate c2 = convertToMap(x,y,16);
                    clickOnCityScreen(c.lat,c.lon, c2.lat, c2.lon);
                }
            }
            break;
            default:
            break;
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
