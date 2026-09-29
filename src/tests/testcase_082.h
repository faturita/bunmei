#ifndef TESTCASE_082_H
#define TESTCASE_082_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "testcase.h"

class TestCase_082 : public TestCase
{
protected:
    bool isdone=false;
    bool haspassed=false;
    std::string message;
    std::string savename;
public:
    TestCase_082();
    virtual ~TestCase_082();

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

#endif // TESTCASE_082_H
