#ifndef TESTCASE_051_H
#define TESTCASE_051_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "testcase.h"

class TestCase_051 : public TestCase
{
protected:
    bool isdone=false;
    bool haspassed=false;
    std::string message;
    int galleonid, triremeid, galleyid, warriorid;
public:
    TestCase_051();
    virtual ~TestCase_051();

    virtual void init();
    virtual int check(int year);

    virtual std::string title();
    virtual int number();

    virtual bool done();
    virtual bool passed();
    virtual std::string failedMessage();
};

#endif // TESTCASE_051_H
