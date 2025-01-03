//  TestCase_001.cpp
//  bunmei
//
//  Created by Rodrigo Ramele on 07/09/2021
//

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "TestCase_001.h"

TestCase_001::TestCase_001()
{

}

TestCase_001::~TestCase_001()
{

}

int TestCase_001::number()
{
    return 0;
}

void TestCase_001::init()
{

}

int TestCase_001::check(int year)
{
    return 0;
}
std::string TestCase_001::title()
{
    return std::string("Generic Test Case");

}

bool TestCase_001::done()
{
    return isdone;
}
bool TestCase_001::passed()
{
    return false;
}
std::string TestCase_001::failedMessage()
{
    return message;
}

TestCase *pickTestCase(int testcase)
{
    return new TestCase_001();
}