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
    virtual int cost(int r_id);
};


#endif   //MARKET_H