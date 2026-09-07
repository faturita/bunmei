#ifndef SETTLER_H
#define SETTLER_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"


class Settler : public Unit, public Shippable
{
    public:
    Settler();
    int getSubType();
    bool canBuildCity();
    int getId() override;
    const char* getName() override;
    virtual int getConsumptionRate(int r_id) override;
};

class SettlerFactory : public BuildableFactory
{
    public:
    SettlerFactory();
    Settler* create();
    virtual std::vector<int> getRequiredResources();
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources);
};


#endif   // SETTLER_H


        