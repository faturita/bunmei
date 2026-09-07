#ifndef SPY_H
#define SPY_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"

    
class Spy : public Unit, public Shippable
{
    public:
    Spy();
    int getSubType();
    int getId() override;
    const char* getName() override;
};

class SpyFactory : public BuildableFactory
{
    public:
    SpyFactory();
    Spy* create();
    virtual std::vector<int> getRequiredResources();
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources);
};

#endif   // SPY_H