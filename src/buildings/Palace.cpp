#include "Palace.h"

Palace::Palace()
{
    strcpy(name,"Palace");
    strncpy(this->assetname,"assets/assets/city/palace.png",256);
}

int Palace::getSubType()
{
    return BUILDING_PALACE;
}

// --------------------------------------------------------
Buildable* PalaceFactory::create()
{
    Palace* p = new Palace();
    return p;
}

PalaceFactory::PalaceFactory()
{
    strncpy(this->name,"Palace",256);
}

std::vector<int> PalaceFactory::getRequiredResources()
{
    std::vector<int> requiredResources;
    requiredResources.push_back(SHIELDS);
    return requiredResources;
}

std::vector<Resource*> PalaceFactory::fullfillment(std::unordered_map<int, Resource*> availableResources)
{
    std::vector<Resource*> consumedResources;
    if (availableResources[SHIELDS] && availableResources[SHIELDS]->amount >= 10000)
    {
        consumedResources.push_back(new Resource{SHIELDS, 10000});
    }
    return consumedResources;
}

