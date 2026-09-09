#ifndef TESTCASE_059_H
#define TESTCASE_059_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "testcase.h"

class TestCase_059 : public TestCase
{
protected:
    bool isdone=false;
    bool haspassed=false;
    std::string message;
    int cityid;
public:
    TestCase_059();
    virtual ~TestCase_059();

    virtual void init();
    virtual int check(int year);

    virtual std::string title();
    virtual int number();

    virtual bool done();
    virtual bool passed();
    virtual std::string failedMessage();
};

#endif // TESTCASE_059_H
