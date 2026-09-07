#include "../codes.h"
#include "Collosseum.h"

Collosseum::Collosseum()
{
    strcpy(name,"Collosseum");
    strncpy(this->assetname,"assets/assets/city/collosseum.png",256);
}

int Collosseum::getSubType()
{
    return BUILDING_COLLOSSEUM;
}

// --------------------------------------------------------

CollosseumFactory::CollosseumFactory()
{
    strncpy(this->name,"Collosseum",256);
    addDependencyCode(TECH_WRITING);
}

Buildable* CollosseumFactory::create()
{
    Collosseum* b = new Collosseum();
    return b;
}

std::vector<int> CollosseumFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> CollosseumFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 50)
    {
        consumedResources.push_back(new Resource{SHIELDS, 50});
    }
    return consumedResources;
}

