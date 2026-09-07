#ifndef MARKET_H
#define MARKET_H

#include "Building.h"
    
class Market : public Building
{
    public:
    Market();
    int getSubType() override;
};

class MarketFactory : public BuildableFactory
{
    public:
    MarketFactory();
    virtual Buildable* create();
    virtual std::vector<int> getRequiredResources();
    virtual std::vector<Resource*> fullfillment(std::unordered_map<int, Resource*> availableResources);
};


#endif   //MARKET_H