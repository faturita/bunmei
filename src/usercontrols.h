#ifndef USERCONTROLS_H
#define USERCONTROLS_H
#include <assert.h>
#include <string>
#include <iostream>

#include "commandorder.h"

#define CONTROLLING_NONE (size_t)0

class Controller
{
public:

    int cityid = -1;

    // Index to Observable interfaces.
    int camera;

    // Which view mode is currently active.
    int view=1;

    struct controlregister registers;

    // Custom parameters that can be entered from controller.
    float param[10];

    bool pause=false;

    bool pp;

    bool finish=false;

    bool teletype=false;

    bool landOwnership = false;

    std::string str;

    float targetX, targetY, targetZ;

    int slider=0;

    void reset()
    {
        registers.roll=registers.pitch=registers.precesion=registers.bank=0;registers.yaw=0;
        registers.thrust=0;
    };

    void stabilize()
    {
        registers.roll=registers.pitch=registers.precesion=registers.bank=registers.yaw=0;
    }

    void interrupt()
    {
        finish=true;
    }

    bool isInterrupted()
    {
        return finish;
    };

    bool isTeletype()
    {
        return teletype;
    };

};


void handleKeypress(unsigned char key, int x, int y);

void processMouseEntry(int state) ;

void processMouse(int button, int state, int x, int y);


void processMouseActiveMotion(int x, int y);

// Movement of the mouse alone.
void processMousePassiveMotion(int x, int y);

#endif // USERCONTROLS_H
