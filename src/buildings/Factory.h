#ifndef FACTORY_H
#define FACTORY_H

#include "Building.h"

class Factory : public Building
{
    public:
    Factory();
    int getSubType() override;
    virtual int getProductionRate(int r_id);
    virtual int getConsumptionRate(int r_id);
};

class FactoryFactory : public BuildableFactory
{
    public:
    FactoryFactory();
    virtual Buildable* create();
    virtual std::vector<int> getRequiredResources();
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources);
};


#endif   //FACTORY_H