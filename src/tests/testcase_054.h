#ifndef TESTCASE_054_H
#define TESTCASE_054_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "testcase.h"

class TestCase_054 : public TestCase
{
protected:
    bool isdone=false;
    bool haspassed=false;
    std::string message;
    int settlerid;
    int enemycityid;
public:
    TestCase_054();
    virtual ~TestCase_054();

    virtual void init();
    virtual int check(int year);

    virtual std::string title();
    virtual int number();

    virtual bool done();
    virtual bool passed();
    virtual std::string failedMessage();
};

#endif // TESTCASE_054_H
