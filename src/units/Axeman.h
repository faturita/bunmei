#ifndef AXEMAN_H
#define AXEMAN_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"


class Axeman : public Unit, public Shippable
{
    public:
    Axeman();
    int getSubType();
    int getId() override;
    const char* getName() override;
};

class AxemanFactory : public BuildableFactory
{
    public:
    AxemanFactory();
    Axeman* create();
    virtual int cost(int r_id);
};

#endif   // AXEMAN_H