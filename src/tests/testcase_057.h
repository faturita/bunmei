#ifndef TESTCASE_057_H
#define TESTCASE_057_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "testcase.h"

class TestCase_057 : public TestCase
{
protected:
    bool isdone=false;
    bool haspassed=false;
    std::string message;
    int cityid;
public:
    TestCase_057();
    virtual ~TestCase_057();

    virtual void init();
    virtual int check(int year);

    virtual std::string title();
    virtual int number();

    virtual bool done();
    virtual bool passed();
    virtual std::string failedMessage();
};

#endif // TESTCASE_057_H
