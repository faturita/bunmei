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
#include "coordinator.h"
#include "usercontrols.h"
#include "messages.h"
#include "savegame.h"


extern Coordinator coordinator;
extern std::unordered_map<int, City*> cities;
extern std::unordered_map<int, Unit*> units;
extern Map map;

// Mouse offset for camera zoom in and out.
int _xoffset = 0;
int _yoffset = 0;

bool goToMode = false;

Controller controller;

extern int year;

void handleKeypress(unsigned char key, int x, int y) {

    if (controller.isTeletype())
    {
        if (key == 13) // Enter key
        {

            if (controller.str.find("save")!=std::string::npos)
            {
                std::string savegamefilename;

                if (controller.str.length()<=4)
                {
                    savegamefilename = "savegame.dat";
                } else {
                    savegamefilename = controller.str.substr(5);
                }
                savegame(savegamefilename.c_str());
            }


            controller.teletype = false;
            message(year, coordinator.a_f_id, controller.str.c_str());

            controller.str.clear();
        }
        else if (key == 127) // Backspace key
        {
            if (!controller.str.empty())
            {
                controller.str.pop_back();
                fflush(stdout);
            }
        }
        else
        {
            controller.str += key;
        }
    } else

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
        case '!':controller.view = 1;break;
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
        case 'r':controller.registers.bank+=1.0f;break;
        case 'v':controller.registers.bank-=1.0f;break;
        case ' ':coordinator.endofturn=true;break;
        case 't':controller.teletype=true;break;
        case 'l':controller.showLandOwnership = !controller.showLandOwnership;break;
        case 'G':
        {
             if (units.find(coordinator.a_u_id) != units.end())
            {
                goToMode = true;
            }           
        }
        break;
        case 'C':
        {
            if (units.find(coordinator.a_u_id) != units.end())
            {
                coordinate c(units[coordinator.a_u_id]->latitude,units[coordinator.a_u_id]->longitude);
                c = map.to_screen(c.lat,c.lon);
                centermapinmap(c.lat, c.lon);
                resetzoom();
            }
        }
        break;
        case 9:
        {
            coordinator.a_u_id = nextMovableUnitId(coordinator.a_f_id);
            printf("Controlling %d\n",coordinator.a_u_id);
        }
        break;
        case 'b':
        {
            if (units.find(coordinator.a_u_id) != units.end())
            {
                if (units[coordinator.a_u_id]->canBuildCity())
                {
                    CommandOrder co;
                    co.command = Command::BuildCityOrder;
                    coordinator.push(co);
                }
            }
        }
        break;
        case 'D':
        {
            CommandOrder co;
            co.command = Command::DisbandUnitOrder;
            coordinator.push(co);
        }
        break;
        case 'F':
        {
            CommandOrder co;
            co.command = Command::FortifyUnitOrder;
            coordinator.push(co);
        }
        break;
        case 'S':
        {
            CommandOrder co;
            co.command = Command::SentryUnitOrder;
            coordinator.push(co);
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
                        if (units.find(coordinator.a_u_id) != units.end())
                        {
                            coordinate co = getCurrentCenter();
                            units[coordinator.a_u_id]->goTo(co.lat,co.lon);
                        }
                        goToMode = false;
                    }

                    if (specialKey == GLUT_ACTIVE_SHIFT)
                    {
                        zoommapin();
                    } else {
                        for (auto& [k,c] : cities) 
                        {
                            coordinate co = getCurrentCenter();
                            // @NOTE: We needed to center the map always in the same value for the cities and transform that
                            //    into the screen coordinates.
                            coordinate c2 = map.to_screen(co.lat,co.lon);
                            centermapinmap(c2.lat, c2.lon);
                            if (co.lat == c->latitude && co.lon == c->longitude)
                            {
                                if (coordinator.a_f_id == c->faction)
                                {
                                    printf("City %s %d %d,%d\n",c->name, c->id, c->latitude, c->longitude);
                                    controller.view = 2;
                                    controller.cityid = c->id;
                                    break;
                                } else {
                                    printf("This is not your city\n"); //@FIXME debug message
                                }
                            }
                        }

                        for (auto& [k,u] : units) 
                        {
                            coordinate co = getCurrentCenter();
                            if (co.lat == u->latitude && co.lon == u->longitude)
                            {
                                if (coordinator.a_f_id == u->faction && u->availablemoves>0)
                                {
                                    printf("Unit %s %d %d,%d\n",u->name, u->id, u->latitude, u->longitude);
                                    coordinator.a_u_id = u->id;

                                    if (u->isFortified())
                                    {
                                        u->packUp();
                                    }

                                    if (u->isSentry())
                                    {
                                        u->wakeUp();
                                    }


                                    break;
                                } else {
                                    printf("This is not your unit\n"); //@FIXME debug message
                                }
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
