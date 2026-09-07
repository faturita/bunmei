#ifndef WAREHOUSE_H
#define WAREHOUSE_H

#include "Building.h"

class Warehouse : public Building
{
    public:
    Warehouse();
    int getSubType() override;
};

class WarehouseFactory : public BuildableFactory
{
    public:
    WarehouseFactory();
    virtual Buildable* create();
    virtual std::vector<int> getRequiredResources();
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources);
};


#endif   //WAREHOUSE_H