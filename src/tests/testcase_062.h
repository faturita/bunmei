#ifndef TESTCASE_062_H
#define TESTCASE_062_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "testcase.h"

class TestCase_062 : public TestCase
{
protected:
    bool isdone=false;
    bool haspassed=false;
    std::string message;
public:
    TestCase_062();
    virtual ~TestCase_062();

    virtual void init();
    virtual int check(int year);

    virtual std::string title();
    virtual int number();

    virtual bool done();
    virtual bool passed();
    virtual std::string failedMessage();
};

#endif // TESTCASE_062_H
