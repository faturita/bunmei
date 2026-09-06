#ifndef TESTCASE_053_H
#define TESTCASE_053_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "testcase.h"

class TestCase_053 : public TestCase
{
protected:
    bool isdone=false;
    bool haspassed=false;
    std::string message;
    int cityid;
    int phase = 0;
public:
    TestCase_053();
    virtual ~TestCase_053();

    virtual void init();
    virtual int check(int year);

    virtual std::string title();
    virtual int number();

    virtual bool done();
    virtual bool passed();
    virtual std::string failedMessage();
};

#endif // TESTCASE_053_H
