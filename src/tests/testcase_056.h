#ifndef TESTCASE_056_H
#define TESTCASE_056_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "testcase.h"

class TestCase_056 : public TestCase
{
protected:
    bool isdone=false;
    bool haspassed=false;
    std::string message;
    int cityid;
public:
    TestCase_056();
    virtual ~TestCase_056();

    virtual void init();
    virtual int check(int year);

    virtual std::string title();
    virtual int number();

    virtual bool done();
    virtual bool passed();
    virtual std::string failedMessage();
};

#endif // TESTCASE_056_H
