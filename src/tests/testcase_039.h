#ifndef TESTCASE_039_H
#define TESTCASE_039_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "testcase.h"

class TestCase_039 : public TestCase
{
protected:
    bool isdone=false;
    bool haspassed=false;
    std::string message;
    int workerRoadId;
    int workerMineId;
    int workerRailroadId;
    int workerQuarryId;
    int workerCampId;
    int workerDerrickId;
    int workerPlantationId;
public:
    TestCase_039();
    virtual ~TestCase_039();

    // This method is called when the test is initialized.  It should create islands and all the other entities.
    virtual void init();

    // This method is called at each simulation step.  The method should check the completion of the code and returns a return value (0 error).
    virtual int check(int year);

    // Title and number of the testcase.
    virtual std::string title();
    virtual int number();

    virtual bool done();
    virtual bool passed();
    virtual std::string failedMessage();
};

#endif // TESTCASE_039_H
