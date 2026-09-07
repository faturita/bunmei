#include "../codes.h"
#include "../resources.h"
#include "Depot.h"

Depot::Depot()
{
    strcpy(name,"Depot");
    strncpy(this->assetname,"assets/assets/city/depot.png",256);
    perkCodes.push_back(STORAGE_EXPANSION_1);
}

int Depot::getSubType()
{
    return BUILDING_DEPOT;
}

// --------------------------------------------------------
DepotFactory::DepotFactory()
{
    strncpy(this->name,"Depot",256);
    addDependencyCode(TECH_POTTERY);
}

Buildable* DepotFactory::create()
{
    Depot* b = new Depot();
    return b;
}

std::vector<int> DepotFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    requiredResources.push_back(tools);
    return requiredResources;
}

std::vector<Resource*> DepotFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    // Depot requires 100 shields AND 100 tools.
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 100 &&
        availableResources[tools]   && availableResources[tools]->amount   >= 100)
    {
        consumedResources.push_back(new Resource{SHIELDS, 100});
        consumedResources.push_back(new Resource{tools, 100});
    }
    return consumedResources;
}

