#include "../codes.h"
#include "../resources.h"
#include "Factory.h"

Factory::Factory()
{
    strcpy(name,"Factory");
    strncpy(this->assetname,"assets/assets/city/factory.png",256);
}

int Factory::getSubType()
{
    return BUILDING_FACTORY;
}

int Factory::getProductionRate(int r_id)
{
    if (r_id == tools)
    {
        return 1;
    }
    return 0; // Default production rate for other resources
}

int Factory::getConsumptionRate(int r_id)
{
    switch (r_id)
    {
        case COINS:
            return 1;
        case iron:
            return 1;
        default:
            return 0; // Default consumption rate for other resources
    }
}

// --------------------------------------------------------
FactoryFactory::FactoryFactory()
{
    strncpy(this->name,"Factory",256);
    addDependencyCode(TECH_INDUSTRIALIZATION);
}

Buildable* FactoryFactory::create()
{
    Factory* b = new Factory();
    return b;
}

std::vector<int> FactoryFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> FactoryFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 200)
    {
        consumedResources.push_back(new Resource{SHIELDS, 200});
    }
    return consumedResources;
}

