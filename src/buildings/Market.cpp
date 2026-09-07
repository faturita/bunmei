#include "../codes.h"
#include "Market.h"

Market::Market()
{
    strcpy(name,"Market");
    strncpy(this->assetname,"assets/assets/city/market.png",256);
}

int Market::getSubType()
{
    return BUILDING_MARKET;
}

// --------------------------------------------------------
MarketFactory::MarketFactory()
{
    strncpy(this->name,"Market",256);
    addDependencyCode(TECH_CURRENCY);
}

Buildable* MarketFactory::create()
{
    Market* b = new Market();
    return b;
}

std::vector<int> MarketFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> MarketFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 80)
    {
        consumedResources.push_back(new Resource{SHIELDS, 80});
    }
    return consumedResources;
}

