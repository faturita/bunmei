#ifndef GALLEY_H
#define GALLEY_H

#include <iostream>
#include "Unit.h"


class Galley : public Unit
{
    public:
    Galley();
    int getSubType();
    MOVEMENT_TYPE virtual getMovementType();
};

class GalleyFactory : public BuildableFactory
{
    public:
    GalleyFactory();
    Galley* create();
    virtual std::vector<int> getRequiredResources();
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources);
};

#endif   // GALLEY_H