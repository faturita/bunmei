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
    virtual std::vector<int> getRequiredResources();
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources);
};


#endif   //DEPOT_H