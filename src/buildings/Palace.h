#ifndef PALACE_H
#define PALACE_H

#include "Building.h"

class Palace : public Building
{
    public:
    Palace();
    int getSubType() override;
};

class PalaceFactory : public BuildableFactory
{
    public:
    PalaceFactory();
    virtual Buildable* create();
    virtual std::vector<int> getRequiredResources();
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources);
};

#endif   //PALACE_H