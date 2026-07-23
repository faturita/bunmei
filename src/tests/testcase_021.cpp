//  TestCase_021.cpp
//  bunmei
//
//  Created by Claude on 21/07/2026
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <deque>
#include <iterator>
#include <iostream>

#include "../map.h"
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../map.h"
#include "../coordinator.h"
#include "../engine.h"
#include "../tiles.h"
#include "../usercontrols.h"
#include "../dialog.h"

#include "testcase_021.h"

// Task #2 (dialog.h/dialog.cpp): a modal Yes/No-style dialog driven by controller.query
// (querydialog).  This drives clickOnDialog() directly (bypassing GLUT mouse events, same
// approach as other testcases) at two points: a hit on option 1 ("No.") must fire
// query.selected(1) and close the dialog; once closed, a second click must NOT reopen it or
// fire the lambda again.  The click coordinates are derived from dialog.cpp's own layout
// constants (PADDING=16, LINE_HEIGHT=30, tile-snapped box height) so this test breaks loudly
// if that layout ever changes shape.

extern Map map;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;
extern Tiles tiles;

extern float mapzoom;

extern Coordinator coordinator;
extern Controller controller;

extern int REAL_SCREEN_WIDTH;
extern int REAL_SCREEN_HEIGHT;

#define TEST_MAPSIZE 1

TestCase_021::TestCase_021()
{

}

TestCase_021::~TestCase_021()
{

}

int TestCase_021::number()
{
    return 21;
}

void TestCase_021::init()
{

    MapDimension dimension = getMapDimension(TEST_MAPSIZE);
    map.init(dimension.halfheight,dimension.halfwidth);

    initTiles(tiles);

    for(int lat=map.minlat;lat<map.maxlat;lat++)
        for (int lon=map.minlon;lon<map.maxlon;lon++)
        {
            map.set(lat,lon) = mapcell(LAND);
        }

    resources.push_back(new Resource(0,0,"assets/assets/city/food.png","Food"));

    Faction *faction = new Faction();
    faction->id = 0;
    strcpy(faction->name,"Vikings");
    faction->red = 255;
    faction->green = 0;
    faction->blue = 0;
    faction->autoPlayer = false;

    factions.push_back(faction);

    mapzoom = 1;
    zoommapin();
    centermapinmap(0,0);
    coordinator.a_f_id = 0;
    coordinator.a_u_id = 0;

}

int TestCase_021::check(int year)
{

    ticks++;

    if (ticks == 300)
    {
        controller.query.active = true;
        controller.query.message = "Test?";
        controller.query.options = {"Yes.", "No."};
        controller.query.selected = [this](int i){ chosenOption = i; };

        // Reproduce dialog.cpp's layout math for a 1-line message + 2 options, so the click
        // lands squarely on option index 1 ("No.").  X: the box is horizontally centered on
        // SCREEN_WIDTH, so the screen center is always inside it regardless of text width.
        const float PADDING = 16.0f, LINE_HEIGHT = 25.0f;
        float rawHeight = 2*PADDING + LINE_HEIGHT*(1 + 2);            // 1 message line, 2 options
        float boxHeight = ceil(rawHeight/16.0f) * 16.0f;               // tile-snapped
        float boxTop = (SCREEN_HEIGHT - boxHeight) / 2.0f;
        float optionsTop = boxTop + PADDING + LINE_HEIGHT*1;
        float targetY = optionsTop + LINE_HEIGHT*1 + LINE_HEIGHT/2.0f; // middle of option index 1's row

        int rawX = (int)(0.5f * REAL_SCREEN_WIDTH);
        int rawY = (int)(targetY / SCREEN_HEIGHT * REAL_SCREEN_HEIGHT);

        clickOnDialog(controller.query, rawX, rawY);
    }

    if (ticks == 400)
    {
        if (chosenOption != 1)
        {
            isdone = true;
            haspassed = false;
            char msg[256];
            sprintf(msg,"Click on option 1 (\"No.\") did not select it: chosenOption=%d.",chosenOption);
            message = std::string(msg);
            return 0;
        }
        if (controller.query.active)
        {
            isdone = true;
            haspassed = false;
            message = std::string("Dialog is still active after an option was selected.");
            return 0;
        }

        // A further click (now that the dialog is closed) must be a no-op: no dialog to hit.
        clickOnDialog(controller.query, (int)(0.5f*REAL_SCREEN_WIDTH), (int)(0.5f*REAL_SCREEN_HEIGHT));

        if (chosenOption != 1)
        {
            isdone = true;
            haspassed = false;
            message = std::string("A click after the dialog closed changed chosenOption.");
            return 0;
        }

        isdone = true;
        haspassed = true;
    }

    return 0;
}
std::string TestCase_021::title()
{
    return std::string("Dialog (task #2): clicking an option fires selected() with its index and closes the dialog.");

}

bool TestCase_021::done()
{
    return isdone;
}
bool TestCase_021::passed()
{
    return haspassed;
}
std::string TestCase_021::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_021();
}
