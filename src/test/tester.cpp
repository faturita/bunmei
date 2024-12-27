
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

#include "../usercontrols.h"
#include "testcase.h"

extern int year;
extern Controller controller;

TestCase *t = NULL;

void update(int value);

void initWorldModelling(int testcase)
{

    t = pickTestCase(testcase);

    t->init();

    controller.faction = 0;
    controller.view = 1;

}

void worldStep(int value)
{

    t->check(year);

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