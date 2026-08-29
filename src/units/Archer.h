#ifndef ARCHER_H
#define ARCHER_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"


class Archer : public Unit, public Shippable
{
    public:
    Archer();
    int getSubType() override;
};

class ArcherFactory : public BuildableFactory
{
    public:
    ArcherFactory();
    Archer* create();
    virtual int cost(int r_id);
};

#endif   // ARCHER_H