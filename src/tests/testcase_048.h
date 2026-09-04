#ifndef TESTCASE_048_H
#define TESTCASE_048_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "testcase.h"

class TestCase_048 : public TestCase
{
protected:
    bool isdone=false;
    bool haspassed=false;
    std::string message;
    int cityid;
    int homeid;
    int wagonid;
    int warriorid;
    int t0 = -1;   // tick engageTrade fired; buy/sell run a few frames later so the real
                   // drawScene render path (view==4) exercises the commerce screen first.
public:
    TestCase_048();
    virtual ~TestCase_048();

    virtual void init();
    virtual int check(int year);

    virtual std::string title();
    virtual int number();

    virtual bool done();
    virtual bool passed();
    virtual std::string failedMessage();
};

#endif // TESTCASE_048_H
