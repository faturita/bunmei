#ifndef HORSEMAN_H
#define HORSEMAN_H

#include <iostream>
#include "Unit.h"


class Horseman : public Unit
{
    public:
    Horseman();

    void draw();
    bool canBuildCity();
};

class HorsemanFactory : public BuildableFactory
{
    public:
    HorsemanFactory();
    Horseman* create();
    virtual int cost(int r_id);
};

#endif   // HORSEMAN_H