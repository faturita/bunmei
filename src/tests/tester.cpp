#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdarg.h>
#include <math.h>

#include <cassert>
#include "../units/Unit.h"
#include "../City.h"
#include "../Faction.h"
#include "../resources.h"
#include "../coordinator.h"
#include "../map.h"
#include "../units/Settler.h"
#include "../engine.h"


#include "../usercontrols.h"
#include "../messages.h"
#include "testcase.h"

extern Controller controller;
extern Coordinator coordinator;

extern std::unordered_map<int, std::string> tiles;
extern Map map;
std::unordered_map<int, std::vector<int>> resourcesxbioma;

extern std::unordered_map<int,std::queue<std::string>> citynames;
extern std::unordered_map<int, Unit*> units;
extern std::unordered_map<int, City*> cities;
extern std::vector<Faction*> factions;
extern std::vector<Resource*> resources;

extern int year;

TestCase *t = NULL;

void update(int value);         // Will be linked with bunmei.update()


void initMap()
{

}

void initResources()
{

}

void initWorldModelling()
{
    t = pickTestCase(0);

    t->init();

    coordinator.a_f_id = 0;
    controller.view = 1;
}



long unsigned timer = 0;

void worldStep(int value)
{
    timer++;
    update(value);

    long unsigned starttime = 200;

    if (timer == starttime)
    {
        message(year, -1, "TC%03d: %s", t->number(), t->title().c_str());
    }

    t->check(timer);

    if (t->done())
    {
        if (t->passed())
        {
            printf("Test Passed\n");
            exit(1);
        }
        else
        {
            char msg[256];
            sprintf(msg, "Test Failed: %s\n", t->failedMessage().c_str());
            printf(msg);
            exit(0);
        }
    }
}
