#ifndef COLLOSSEUM_H
#define COLLOSSEUM_H

#include "Building.h"

class Collosseum : public Building
{
    public:
    Collosseum();
    int getSubType() override;
};

// --------------------------------------------------------
class CollosseumFactory : public BuildableFactory
{
    public:
    CollosseumFactory();
    virtual Buildable* create();
    virtual std::vector<int> getRequiredResources();
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources);
};


#endif   //COLLOSSEUM_H