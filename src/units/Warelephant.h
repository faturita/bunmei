#ifndef WARELEPHANT_H
#define WARELEPHANT_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"


class Warelephant : public Unit, public Shippable
{
    public:
    Warelephant();
    int getSubType();
    int getId() override;
    const char* getName() override;
};

class WarelephantFactory : public BuildableFactory
{
    public:
    WarelephantFactory();
    Warelephant* create();
    virtual std::vector<int> getRequiredResources();
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources);
};

#endif   // WARELEPHANT_H