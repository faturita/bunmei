#ifndef AXEMAN_H
#define AXEMAN_H

#include <iostream>
#include "Unit.h"


class Axeman : public Unit
{
    public:
    Axeman();

    bool canBuildCity();
};

class AxemanFactory : public BuildableFactory
{
    public:
    AxemanFactory();
    Axeman* create();
    virtual int cost(int r_id);
};

#endif   // AXEMAN_H