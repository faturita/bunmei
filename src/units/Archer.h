#ifndef ARCHER_H
#define ARCHER_H

#include <iostream>
#include "Unit.h"


class Archer : public Unit
{
    public:
    Archer();

    void draw();
    bool canBuildCity();
};

class ArcherFactory : public BuildableFactory
{
    public:
    ArcherFactory();
    Archer* create();
    virtual int cost(int r_id);
};

#endif   // ARCHER_H