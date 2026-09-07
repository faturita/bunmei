#ifndef GRANARY_H
#define GRANARY_H

#include "Building.h"

class Granary : public Building
{
    public:
    Granary();
    int getSubType() override;
};

class GranaryFactory : public BuildableFactory
{
    public:
    GranaryFactory();
    virtual Buildable* create();
    virtual std::vector<int> getRequiredResources();
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources);
};


#endif   //GRANARY_H