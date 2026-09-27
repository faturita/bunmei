#ifndef SLAKE_H
#define SLAKE_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"


class Slake : public Unit, public Shippable
{
    public:
    Slake();
    int getSubType();
    int getId() override;
    const char* getName() override;
};

class SlakeFactory : public BuildableFactory
{
    public:
    SlakeFactory();
    Slake* create();
    virtual std::vector<int> getRequiredResources();
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources);
};

#endif   // SLAKE_H