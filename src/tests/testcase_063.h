#ifndef TESTCASE_063_H
#define TESTCASE_063_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "testcase.h"

class TestCase_063 : public TestCase
{
protected:
    bool isdone=false;
    bool haspassed=false;
    std::string message;
public:
    TestCase_063();
    virtual ~TestCase_063();

    virtual void init();
    virtual int check(int year);

    virtual std::string title();
    virtual int number();

    virtual bool done();
    virtual bool passed();
    virtual std::string failedMessage();
};

#endif // TESTCASE_063_H
