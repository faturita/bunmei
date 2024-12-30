#ifndef SETTLER_H
#define SETTLER_H

#include <iostream>
#include "Unit.h"


class Settler : public Unit
{
    public:
    Settler();

    void draw();
    bool canBuildCity();
};

class SettlerFactory : public BuildableFactory
{
    public:
    Settler* create();
    virtual int cost(int r_id);
};


#endif   // SETTLER_H


        