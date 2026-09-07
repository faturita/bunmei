#ifndef ARCHER_H
#define ARCHER_H

#include <iostream>
#include "../shippable.h"
#include "Unit.h"


class Archer : public Unit, public Shippable
{
    public:
    Archer();
    int getSubType() override;
    int getId() override;
    const char* getName() override;
};

class ArcherFactory : public BuildableFactory
{
    public:
    ArcherFactory();
    Archer* create();
    virtual std::vector<int> getRequiredResources();
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources);
};

#endif   // ARCHER_H