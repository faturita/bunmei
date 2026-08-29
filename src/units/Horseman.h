#ifndef HORSEMAN_H
#define HORSEMAN_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"


class Horseman : public Unit, public Shippable
{
    public:
    Horseman();
    int getSubType();
};

class HorsemanFactory : public BuildableFactory
{
    public:
    HorsemanFactory();
    Horseman* create();
    virtual int cost(int r_id);
};

#endif   // HORSEMAN_H