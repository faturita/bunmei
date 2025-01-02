#ifndef TRIREME_H
#define TRIREME_H

#include <iostream>
#include "Unit.h"


class Trireme : public Unit
{
    public:
    Trireme();

    void draw();
    bool canBuildCity();
    MOVEMENT_TYPE virtual getMovementType();
};

class TriremeFactory : public BuildableFactory
{
    public:
    TriremeFactory();
    Trireme* create();
    virtual int cost(int r_id);
};

#endif   // TRIREME_H