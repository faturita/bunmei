#ifndef TESTCASE_071_H
#define TESTCASE_071_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "testcase.h"

class TestCase_071 : public TestCase
{
protected:
    bool isdone=false;
    bool haspassed=false;
    std::string message;
    int warriorid;
    int enemyid;
    int workerid;
public:
    TestCase_071();
    virtual ~TestCase_071();

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

#endif // TESTCASE_071_H
