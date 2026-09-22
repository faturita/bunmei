#ifndef OBSERVATORY_
#define OBSERVATORY_H

#include "Building.h"
    
class Observatory : public Building
{
    public:
    Observatory();
    int getSubType() override;
};

class ObservatoryFactory : public BuildableFactory
{
    public:
    ObservatoryFactory();
    virtual Buildable* create();
    virtual std::vector<int> getRequiredResources();
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources);
};


#endif   //OBSERVATORY_H