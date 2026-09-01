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
    virtual int cost(int r_id);
};


#endif   //WAREHOUSE_H