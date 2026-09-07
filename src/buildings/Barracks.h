#ifndef BARRACKS_H
#define BARRACKS_H

#include "Building.h"

class Barracks : public Building
{
    public:
    Barracks();
    int getSubType() override;
};

class BarracksFactory : public BuildableFactory
{
    public:
    BarracksFactory();
    virtual Buildable* create();
    virtual std::vector<int> getRequiredResources();
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources);
};


#endif   //BARRACKS_H