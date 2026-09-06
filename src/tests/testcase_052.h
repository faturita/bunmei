#ifndef TESTCASE_052_H
#define TESTCASE_052_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "testcase.h"

class TestCase_052 : public TestCase
{
protected:
    bool isdone=false;
    bool haspassed=false;
    std::string message;
public:
    TestCase_052();
    virtual ~TestCase_052();

    virtual void init();
    virtual int check(int year);

    virtual std::string title();
    virtual int number();

    virtual bool done();
    virtual bool passed();
    virtual std::string failedMessage();
};

#endif // TESTCASE_052_H
