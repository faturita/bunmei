#ifndef DEPOT_H
#define DEPOT_H

#include "Building.h"

class Depot : public Building
{
    public:
    Depot();
    int getSubType() override;
};

class DepotFactory : public BuildableFactory
{
    public:
    DepotFactory();
    virtual Buildable* create();
    virtual int cost(int r_id);
};


#endif   //DEPOT_H