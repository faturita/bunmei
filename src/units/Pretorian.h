#ifndef PRETORIAN_H
#define PRETORIAN_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"


class Pretorian : public Unit, public Shippable
{
    public:
    Pretorian();
    int getSubType();
    int getId() override;
    const char* getName() override;
};

class PretorianFactory : public BuildableFactory
{
    public:
    PretorianFactory();
    Pretorian* create();
    virtual std::vector<int> getRequiredResources();
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources);
};

#endif   // PRETORIAN_H