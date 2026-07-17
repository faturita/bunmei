#ifndef TESTCASE_015_H
#define TESTCASE_015_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "testcase.h"

class TestCase_015 : public TestCase
{
protected:
    bool isdone=false;
    bool haspassed=false;
    std::string message;
    std::vector<unsigned char> baseline;
    std::vector<unsigned char> straightroad;
public:
    TestCase_015();
    virtual ~TestCase_015();

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



#endif // TESTCASE_015_H
