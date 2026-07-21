#ifndef PRETORIAN_H
#define PRETORIAN_H

#include <iostream>
#include "Unit.h"


class Pretorian : public Unit
{
    public:
    Pretorian();
    int getSubType();
};

class PretorianFactory : public BuildableFactory
{
    public:
    PretorianFactory();
    Pretorian* create();
    virtual int cost(int r_id);
};

#endif   // PRETORIAN_H