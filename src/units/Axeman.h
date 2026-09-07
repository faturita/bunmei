#ifndef AXEMAN_H
#define AXEMAN_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"


class Axeman : public Unit, public Shippable
{
    public:
    Axeman();
    int getSubType();
    int getId() override;
    const char* getName() override;
};

class AxemanFactory : public BuildableFactory
{
    public:
    AxemanFactory();
    Axeman* create();
    virtual std::vector<int> getRequiredResources();
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources);
};

#endif   // AXEMAN_H