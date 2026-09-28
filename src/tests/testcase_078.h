#ifndef TESTCASE_078_H
#define TESTCASE_078_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "testcase.h"

class TestCase_078 : public TestCase
{
protected:
    bool isdone=false;
    bool haspassed=false;
    std::string message;
    int warriorid, debtorid;
public:
    TestCase_078();
    virtual ~TestCase_078();

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

#endif // TESTCASE_078_H
