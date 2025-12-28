#ifndef GALLEY_H
#define GALLEY_H

#include <iostream>
#include "Unit.h"


class Galley : public Unit
{
    public:
    Galley();
    int getSubType();
    MOVEMENT_TYPE virtual getMovementType();
};

class GalleyFactory : public BuildableFactory
{
    public:
    GalleyFactory();
    Galley* create();
    virtual int cost(int r_id);
};

#endif   // GALLEY_H