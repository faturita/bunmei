#ifndef WARELEPHANT_H
#define WARELEPHANT_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"


class Warelephant : public Unit, public Shippable
{
    public:
    Warelephant();
    int getSubType();
};

class WarelephantFactory : public BuildableFactory
{
    public:
    WarelephantFactory();
    Warelephant* create();
    virtual int cost(int r_id);
};

#endif   // WARELEPHANT_H