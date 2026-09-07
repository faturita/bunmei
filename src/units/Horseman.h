#ifndef HORSEMAN_H
#define HORSEMAN_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"


class Horseman : public Unit, public Shippable
{
    public:
    Horseman();
    int getSubType();
    int getId() override;
    const char* getName() override;
};

class HorsemanFactory : public BuildableFactory
{
    public:
    HorsemanFactory();
    Horseman* create();
    virtual std::vector<int> getRequiredResources();
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources);
};

#endif   // HORSEMAN_H