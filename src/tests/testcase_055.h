#ifndef TESTCASE_055_H
#define TESTCASE_055_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "testcase.h"

class TestCase_055 : public TestCase
{
protected:
    bool isdone=false;
    bool haspassed=false;
    std::string message;
    int triremeid;
    int warriorid;
    int enemycityid;
public:
    TestCase_055();
    virtual ~TestCase_055();

    virtual void init();
    virtual int check(int year);

    virtual std::string title();
    virtual int number();

    virtual bool done();
    virtual bool passed();
    virtual std::string failedMessage();
};

#endif // TESTCASE_055_H
