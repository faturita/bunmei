#ifndef TESTCASE_047_H
#define TESTCASE_047_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "testcase.h"

class TestCase_047 : public TestCase
{
protected:
    bool isdone=false;
    bool haspassed=false;
    std::string message;
    int warriorid;
public:
    TestCase_047();
    virtual ~TestCase_047();

    virtual void init();
    virtual int check(int year);

    virtual std::string title();
    virtual int number();

    virtual bool done();
    virtual bool passed();
    virtual std::string failedMessage();
};

#endif // TESTCASE_047_H
